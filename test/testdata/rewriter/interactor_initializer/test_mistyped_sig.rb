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

class TestMistypedSig
  extend T::Sig

  include Interactor::Initializer

  sig { params(object: String, paams: T::Hash[T.untyped, T.untyped]).void }
                              #^^^^^ error: Unknown argument name `paams`
  initialize_with :object, :params
                          # ^^^^^^ error-with-dupes: Malformed `sig`. Type not specified for argument `params`
                          # ^^^^^^ error-with-dupes: This function does not have a `sig`
                          # ^^^^^^ error-with-dupes: Use of undeclared variable `@params`

  sig { void }
  def run; end
end
