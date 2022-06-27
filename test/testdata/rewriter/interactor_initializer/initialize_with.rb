# typed: strict

module Interactor::Initializer
  extend T::Helpers
  mixes_in_class_methods ::Interactor::Initializer::ClassMethods

  module ClassMethods
    extend T::Sig

    sig { params(args: T.untyped).void }
    def initialize_with(*args); end
  end
end

class Test
  extend T::Sig

  include Interactor::Initializer

  sig { params(object: String, params: T::Hash[T.untyped, T.untyped], context: T.untyped).void }
  initialize_with :object, :params, :context

  sig { returns(T.nilable(String)) }
  def run
    T.reveal_type(object) # error: Revealed type: `T.nilable(String)`
    T.reveal_type(params) # error: Revealed type: `T.nilable(T::Hash[T.untyped, T.untyped])`
    T.reveal_type(context) # error: Revealed type: `T.untyped`

    object
  end
end

res = Test.for('foo', {}, nil)
T.reveal_type(res) # error: Revealed type: `T.nilable(String)`

class TestWithoutIncludedModule
  extend T::Sig

  sig { params(object: String, params: T::Hash[T.untyped, T.untyped]).void }
  initialize_with :object, :params
end

class TestWithMistypedSig
  extend T::Sig

  include Interactor::Initializer

  sig { params(object: String, paams: T::Hash[T.untyped, T.untyped]).void }
                              #^^^^^ error: Unknown argument name `paams`
  initialize_with :object, :params # error-with-dupes: Malformed `sig`. Type not specified for argument `params`
                          # ^^^^^^ error-with-dupes: This function does not have a `sig`
                          # ^^^^^^ error-with-dupes: Use of undeclared variable `@params`
end

class TestSigNotProvided
  extend T::Sig

  include Interactor::Initializer

  sig { params(object: String).void }
  initialize_with :object, :params # error-with-dupes: Malformed `sig`. Type not specified for argument `params`
                          # ^^^^^^ error-with-dupes: This function does not have a `sig`
                          # ^^^^^^ error-with-dupes: Use of undeclared variable `@params`

  sig { void }
  def run
    T.reveal_type(params) # error: Revealed type: `T.untyped`
  end
end

class TestInitializedWithString
  class TestSigNotProvided
    extend T::Sig

    include Interactor::Initializer

    sig { params(object: String).void } # error: Unknown argument name `object`
    initialize_with 'object'
                  # ^^^^^^^^ error: Argument must be a Symbol
  end
end
