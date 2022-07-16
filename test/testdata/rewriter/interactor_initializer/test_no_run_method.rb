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

class TestNoRunMethod
  extend T::Sig

  include Interactor::Initializer

  sig { params(object: String).void }
  initialize_with :object
end

TestNoRunMethod.for(1) # error: Method `for` does not exist on `T.class_of(TestNoRunMethod)`
