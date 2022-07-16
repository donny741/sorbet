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

class TestSigNotProvided
  extend T::Sig

  include Interactor::Initializer

  sig { params(object: String).void }
  initialize_with :object, :params
                          # ^^^^^^ error-with-dupes: Malformed `sig`. Type not specified for argument `params`
                          # ^^^^^^ error-with-dupes: This function does not have a `sig`
                          # ^^^^^^ error-with-dupes: Use of undeclared variable `@params`

  sig { void }
  def run
    T.reveal_type(params) # error: Revealed type: `T.untyped`
  end
end
