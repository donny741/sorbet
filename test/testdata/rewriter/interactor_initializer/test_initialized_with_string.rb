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

class TestInitializedWithString
  class TestSigNotProvided
    extend T::Sig

    include Interactor::Initializer

    sig { params(object: String).void } # error: Unknown argument name `object`
    initialize_with 'object'
  # ^^^^^^^^^^^^^^^^^^^^^^^^ error-with-dupes: This function does not have a `sig`
                  # ^^^^^^^^ error: Argument must be a Symbol

    sig { void }
    def run; end
  end
end
