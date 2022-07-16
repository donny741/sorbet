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
    T.reveal_type(object) # error: Revealed type: `String`
    T.reveal_type(params) # error: Revealed type: `T::Hash[T.untyped, T.untyped]`
    T.reveal_type(context) # error: Revealed type: `T.untyped`

    object
  end
end

res = Test.for('foo', {}, nil)
T.reveal_type(res) # error: Revealed type: `T.nilable(String)`
