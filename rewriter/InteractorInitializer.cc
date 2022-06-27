#include "rewriter/InteractorInitializer.h"
#include "ast/Helpers.h"
#include "ast/ast.h"
#include "core/Context.h"
#include "core/Names.h"
#include "core/core.h"
#include "core/errors/rewriter.h"
#include "rewriter/Util.h"
#include "rewriter/rewriter.h"

using namespace std;

namespace sorbet::rewriter {

namespace {

pair<core::NameRef, core::LocOffsets> getName(core::MutableContext ctx, ast::ExpressionPtr &name) {
    core::LocOffsets loc;
    core::NameRef res;

    if (auto *lit = ast::cast_tree<ast::Literal>(name)) {
        if (lit->isSymbol()) {
            res = lit->asSymbol();
            loc = lit->loc;
            ENFORCE(ctx.locAt(loc).exists());
            ENFORCE(ctx.locAt(loc).source(ctx).value().size() > 1 && ctx.locAt(loc).source(ctx).value()[0] == ':');
            loc = core::LocOffsets{loc.beginPos() + 1, loc.endPos()};
        }
    }
    if (!res.exists()) {
        if (auto e = ctx.beginError(name.loc(), core::errors::Rewriter::BadAttrArg)) {
            e.setHeader("Argument must be a Symbol");
        }
    }
    return make_pair(res, loc);
}

// This will raise an error if we've given a type that's not what we want
void ensureSafeSig(core::MutableContext ctx, const core::NameRef attrFun, ast::Send *sig) {
    // Loop down the chain of recv's until we get to the inner 'sig' node.
    auto *block = sig->block();
    auto *body = ast::cast_tree<ast::Send>(block->body);
    auto *cur = body;
    while (cur != nullptr) {
        if (cur->fun == core::Names::typeParameters()) {
            if (auto e = ctx.beginError(sig->loc, core::errors::Rewriter::BadAttrType)) {
                e.setHeader("The type for an `{}` cannot contain `{}`", attrFun.show(ctx), "type_parameters");
            }
            auto &arg = body->getPosArg(0);
            arg = ast::MK::Untyped(arg.loc());
        }
        cur = ast::cast_tree<ast::Send>(cur->recv);
    }
}

ast::ExpressionPtr parsePropType(core::NameRef name, ast::Send *sig) {
    const auto empty = nullptr;

    if (sig == nullptr) {
        return empty;
    }

    auto *block = sig->block();
    auto *body = ast::cast_tree<ast::Send>(block->body);
    auto *cur = body;
    while (cur != nullptr) {
        if (cur->fun == core::Names::params()) {
            auto numKwArgs = cur->numKwArgs();
            for (auto i = 0; i < numKwArgs; ++i) {
                auto &arg = cur->getKwKey(i);

                if (auto *lit = ast::cast_tree<ast::Literal>(arg)) {
                    if (lit->isSymbol()) {
                        core::NameRef typeName = lit->asSymbol();
                        // check if symbol
                        if (typeName == name) {
                            return ASTUtil::dupType(cur->getKwValue(i));
                        }
                    }
                }
            }
        }
        cur = ast::cast_tree<ast::Send>(cur->recv);
    }
    return empty;
}

struct ArgInfo {
    core::NameRef name;
    core::NameRef varName;
    core::NameRef setName;
    core::LocOffsets loc;
    ast::ExpressionPtr type;
};

// bool isT(const ast::ExpressionPtr &expr) {
//     auto *t = ast::cast_tree<ast::UnresolvedConstantLit>(expr);
//     return t != nullptr && t->cnst == core::Names::Constants::T() && ast::MK::isRootScope(t->scope);
// }

// bool isTNilable(const ast::ExpressionPtr &expr) {
//     auto *nilable = ast::cast_tree<ast::Send>(expr);
//     return nilable != nullptr && nilable->fun == core::Names::nilable() && isT(nilable->recv);
// }

// bool isTUntyped(const ast::ExpressionPtr &expr) {
//     auto *send = ast::cast_tree<ast::Send>(expr);
//     return send != nullptr && send->fun == core::Names::untyped() && isT(send->recv);
// }

ArgInfo parseProp(core::MutableContext ctx, ast::ExpressionPtr &prop, ast::Send *sig) {
    ArgInfo ret;

    auto [name, argLoc] = getName(ctx, prop);
    if (!name.exists()) {
        return ret;
    }
    ret.loc = argLoc;
    ret.name = name;
    ret.varName = name.addAt(ctx);
    ret.setName = name.addEq(ctx);
    ret.type = parsePropType(name, sig);

    // Seems to be not needed. Maybe make reader methods returns nilable?
    // if (ret.type != nullptr && !isTNilable(ret.type) && !isTUntyped(ret.type)) {
    //     ret.type = ast::MK::Nilable(ret.loc, std::move(ret.type));
    // }

    return ret;
}

ast::ExpressionPtr mkTypedInitialize(core::LocOffsets loc, vector<ArgInfo> *props) {
    ast::MethodDef::ARGS_store args;
    ast::InsSeq::STATS_store bodyStats;

    for (auto &prop : *props) {
        auto assignmentValue = ast::MK::Local(prop.loc, prop.name);
        if (prop.type != nullptr) {
            assignmentValue = ast::MK::Let(prop.loc, std::move(assignmentValue), prop.type.deepCopy());
        }
        bodyStats.push_back(
            ast::MK::Assign(prop.loc, ast::MK::Instance(prop.loc, prop.varName), std::move(assignmentValue)));
        args.emplace_back(ast::MK::Local(prop.loc, prop.name));
    }
    auto body = ast::MK::InsSeq(loc, std::move(bodyStats), ast::MK::Nil(loc));

    return ast::MK::SyntheticMethod(loc, loc, core::Names::initialize(), std::move(args), std::move(body));
}

vector<ast::ExpressionPtr> mkClassMethods(core::LocOffsets loc, vector<ArgInfo> *props, ast::ExpressionPtr returnType) {
    const vector<core::NameRef> methods = {core::Names::with(), core::Names::for_(), core::Names::run()};

    vector<ast::ExpressionPtr> result;

    // Take location of the real sig ?
    ast::ExpressionPtr sig;
    ast::Send::ARGS_store sigArgs;
    for (auto &prop : *props) {
        if (prop.name.exists() && prop.type != nullptr) {
            sigArgs.emplace_back(ast::MK::Symbol(prop.loc, prop.name));
            sigArgs.emplace_back(prop.type.deepCopy());
        }
    }
    if (returnType != nullptr) {
        sig = ast::MK::Sig(loc, std::move(sigArgs), returnType.deepCopy());
    } else {
        sig = ast::MK::SigVoid(loc, std::move(sigArgs));
    }

    for (auto method : methods) {
        ast::Send::ARGS_store args;
        ast::Send::ARGS_store args2;
        for (auto &prop : *props) {
            args.emplace_back(ast::MK::Local(prop.loc, prop.name));
            args2.emplace_back(ast::MK::Local(prop.loc, prop.name));
        }

        auto newCall = ast::MK::Send(loc, ast::MK::Self(loc), core::Names::new_(), loc, args2.size(), std::move(args2));
        auto runCall = ast::MK::Send0(loc, std::move(newCall), core::Names::run(), loc);

        ast::MethodDef::Flags flags;
        flags.isSelfMethod = true;

        result.insert(result.end(), sig.deepCopy());
        result.insert(result.end(),
                      ast::MK::SyntheticMethod(loc, loc, method, std::move(args), std::move(runCall), flags));
    }

    return result;
}

ast::ExpressionPtr resolveReturnType(ast::ClassDef *classDef) {
    ast::ExpressionPtr *prevStat = nullptr;
    UnorderedMap<void *, vector<ast::ExpressionPtr>> replaceStats;

    // Finding `#run` method
    for (auto &stat : classDef->rhs) {
        auto *methodDef = ast::cast_tree<ast::MethodDef>(stat);
        if (methodDef == nullptr || methodDef->name != core::Names::run()) {
            prevStat = &stat;
            continue;
        }

        // Found `#run`, resolving type from its sig
        auto sig = ASTUtil::castSig(*prevStat);
        if (sig == nullptr) {
            // Method has no valid sig
            break;
        }

        auto *block = sig->block();
        auto body = ast::cast_tree<ast::Send>(block->body);

        while (body->fun != core::Names::returns() && body->fun != core::Names::void_()) {
            body = ast::cast_tree<ast::Send>(body->recv);
        }

        if (body->fun == core::Names::returns()) {
            auto numPosArgs = body->numPosArgs();
            if (numPosArgs != 1) {
                break;
            }

            return ASTUtil::dupType(body->getPosArg(0));
        }
    }

    return nullptr;
}

vector<ast::ExpressionPtr> parseInitializerStatement(core::MutableContext ctx, ast::Send *send, ast::Send *sig,
                                                     ast::ExpressionPtr returnType) {
    vector<ast::ExpressionPtr> stats;
    const auto numPosArgs = send->numPosArgs();
    vector<ArgInfo> props;

    auto loc = send->loc;

    for (auto i = 0; i < numPosArgs; ++i) {
        auto &arg = send->getPosArg(i);
        auto prop = parseProp(ctx, arg, sig);

        if (!prop.name.exists()) {
            continue;
        }

        if (prop.type != nullptr) {
            stats.emplace_back(ast::MK::Sig0(loc, prop.type.deepCopy()));
        }
        auto reader =
            ast::MK::SyntheticMethod0(prop.loc, prop.loc, prop.name, ast::MK::Instance(prop.loc, prop.varName));
        stats.emplace_back(std::move(reader));

        if (prop.type != nullptr) {
            ast::Send::ARGS_store writerSigArgs =
                ast::MK::SendArgs(ast::MK::Symbol(loc, prop.name), prop.type.deepCopy());
            stats.emplace_back(ast::MK::Sig(loc, std::move(writerSigArgs), prop.type.deepCopy()));
        }
        auto body = ast::MK::Assign(loc, ast::MK::Instance(prop.loc, prop.varName), ast::MK::Local(loc, prop.name));
        auto writer = ast::MK::SyntheticMethod1(prop.loc, prop.loc, prop.setName, ast::MK::Local(prop.loc, prop.name),
                                                move(body));
        stats.emplace_back(std::move(writer));

        props.emplace_back(std::move(prop));
    }

    auto classMethods = mkClassMethods(loc, &props, std::move(returnType));

    std::move(classMethods.begin(), classMethods.end(), std::back_inserter(stats));
    stats.insert(stats.begin(), mkTypedInitialize(loc, &props));
    if (sig != nullptr) {
        stats.insert(stats.begin(), sig->deepCopy());
    }

    return stats;
}

} // namespace

void InteractorInitializer::run(core::MutableContext ctx, ast::ClassDef *classDef) {
    ast::ExpressionPtr *prevStat = nullptr;
    UnorderedMap<void *, vector<ast::ExpressionPtr>> replaceStats;
    uint16_t numReplacements;

    for (auto &stat : classDef->rhs) {
        auto *send = ast::cast_tree<ast::Send>(stat);
        if (send == nullptr) {
            continue;
        }

        if (send->fun == core::Names::initializeWith()) {
            vector<ast::ExpressionPtr> empty;

            ast::Send *sig = nullptr;
            if (prevStat) {
                sig = ASTUtil::castSig(*prevStat);
                if (sig != nullptr) {
                    ensureSafeSig(ctx, send->fun, sig);
                }
            }

            auto returnType = resolveReturnType(classDef);
            auto stats = parseInitializerStatement(ctx, send, sig, std::move(returnType));
            numReplacements = stats.size();
            replaceStats[stat.get()] = std::move(stats);
            if (prevStat != nullptr) {
                replaceStats[prevStat->get()] = std::move(empty);
            }
            break;
        }
        prevStat = &stat;
    }

    // this is cargo-culted from Prop.cc.
    auto oldRHS = std::move(classDef->rhs);
    classDef->rhs.clear();
    classDef->rhs.reserve(oldRHS.size() + numReplacements);

    for (auto &stat : oldRHS) {
        auto replacement = replaceStats.find(stat.get());
        if (replacement == replaceStats.end()) {
            classDef->rhs.emplace_back(std::move(stat));
        } else {
            for (auto &newNode : replacement->second) {
                classDef->rhs.emplace_back(std::move(newNode));
            }
        }
    }
}
} // namespace sorbet::rewriter
