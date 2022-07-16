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

class TestNoSig
  include Interactor::Initializer

  initialize_with :object
# ^^^^^^^^^^^^^^^^^^^^^^^ error-with-dupes: This function does not have a `sig`
                 # ^^^^^^ error-with-dupes: This function does not have a `sig`
                 # ^^^^^^ error-with-dupes: Use of undeclared variable `@object`

  def run; end
# ^^^^^^^ error: This function does not have a `sig`
end
