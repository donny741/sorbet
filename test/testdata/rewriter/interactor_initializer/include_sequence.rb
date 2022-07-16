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

module Constants
  TEMPLATE = T.let('string'.freeze, String)
end

# It does not remove statement before initialize_with if it is not a signature
class Test
  extend T::Sig

  include Interactor::Initializer
  include Constants

  initialize_with :object
# ^^^^^^^^^^^^^^^^^^^^^^^ error-with-dupes: This function does not have a `sig`
                 # ^^^^^^ error-with-dupes: This function does not have a `sig`
                 # ^^^^^^ error-with-dupes: Use of undeclared variable `@object`

  sig { returns(String) }
  def run
    TEMPLATE
  end
end
