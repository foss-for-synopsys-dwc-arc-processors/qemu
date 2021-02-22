class QEmuCompiler
  include Compiler

  def initialize()
    @passes = [
      IdentifyQEmuStaticInferedParts,
      SpaghettiCodePass,
      DecomposeExpressions,
      UnfoldCode,
      CreateInternalVars,
    ]
    @translator = QEmuTranslator
  end

end
