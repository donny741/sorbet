#ifndef SORBET_REWRITER_INTERACTOR_INITIALIZER_H
#define SORBET_REWRITER_INTERACTOR_INITIALIZER_H
#include "ast/ast.h"

namespace sorbet::rewriter {

/**
 * This class desugars Interactor::Initializer
 *
 *   initialize_with :foo
 *
 * into
 *
 *   def foo; @foo; end
 *   def foo=(foo); @foo=foo; end
 *   def initialize_with(foo); @foo=foo; end
 *
 * and
 *
 *   initialize_with_keyword_params :foo
 *
 * into
 *
 *   def foo; @foo; end
 *   def foo=(foo); @foo=foo; end
 *   def initialize_with(foo:); @foo=foo; end
 *
 *
 */
class InteractorInitializer final {
public:
    static void run(core::MutableContext ctx, ast::ClassDef *classDef);

    InteractorInitializer() = delete;
};

} // namespace sorbet::rewriter

#endif
