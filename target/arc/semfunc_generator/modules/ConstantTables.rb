module ConstantTables

  TEMP_CREATING_FUNCTIONS = [
    "CarryADD",
    "CarrySUB",
    "OverflowADD",
    "OverflowSUB",
    "getCCFlag",

    "Carry",

    "getCFlag",
    "getMemory",
    # "SignExtend",
    # "getNFlag",
    "getPC",
    "nextInsnAddressAfterDelaySlot",
    "nextInsnAddress",
    "getPCL",
    "unsignedLT",
    "unsignedGE",
    "logicalShiftRight",
    "logicalShiftLeft",
    "arithmeticShiftRight",
    "rotateLeft",
    "rotateRight",
    "getBit",
    "getRegIndex",
    "readAuxReg",
    "extractBits",
    "getRegister",
    "ARC_HELPER",
    # "nextReg",
    "CLZ",
    "CTZ",

    "MAC",
    "MACU",

    "divSigned",
    "divUnsigned",
    "divRemainingSigned",
    "divRemainingUnsigned",
    "getLF",
    "setLF",
    "hasInterrupts",
    "NoFurtherLoadsPending",
    "inKernelMode"
  ]


  DIRECT_TCG_FUNC_TRANSLATIONS = {
    "CLZ" => "tcg_gen_clz_tl",
    "CTZ" => "tcg_gen_ctz_tl",
    "CRLSB" => "tcg_gen_clrsb_tl",
    "SignExtend16to32" => "tcg_gen_ext16s_tl"
  }
end
