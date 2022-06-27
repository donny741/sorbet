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

  sig { params(object: String).void }
  initialize_with :object

  sig { returns(String) }
  def run
    object
  end
end

res = Test.for('foo')
T.reveal_type(res) # error: Revealed type: `String`

class TestRunWithVoidSig
  extend T::Sig

  include Interactor::Initializer

  sig { params(object: String).void }
  initialize_with :object

  sig { void }
  def run; end
end
res = TestRunWithVoidSig.for('foo')
T.reveal_type(res) # error: Revealed type: `Sorbet::Private::Static::Void`

class TestRunWithoutSig
  extend T::Sig

  include Interactor::Initializer

  sig { params(object: String).void }
  initialize_with :object

  def run # error: This function does not have a `sig`
    T.reveal_type(object) # error: Revealed type: `String`
  end
end
res = TestRunWithVoidSig.for('foo')
T.reveal_type(res) # error: Revealed type: `Sorbet::Private::Static::Void`
