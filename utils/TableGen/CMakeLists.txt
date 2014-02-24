set(LLVM_LINK_COMPONENTS Support)

add_tablegen(fracture-tblgen FRACTURE
  FracturePatternlessInstrsEmitter.cpp
  TableGen.cpp
  )
