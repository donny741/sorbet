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

class TestNoIncludedModule
  extend T::Sig

  sig { params(object: String).void } # error: Unused type annotation. No method def before next annotation
             # ^^^^^^ error: Unknown argument name `object`
  initialize_with :object # error: Method `initialize_with` does not exist on `T.class_of(TestNoIncludedModule)`

  sig { void }
  def run; end
end
