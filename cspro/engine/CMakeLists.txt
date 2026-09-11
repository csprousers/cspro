project(engine)

set(CEXENTRY_DIR ${CSPRO_DIR}/engine)
set(ZCAPIO_DIR ${CSPRO_DIR}/zCapiO)
set(ZENTRYO_DIR ${CSPRO_DIR}/zEntryO)
set(ZISSALIB_DIR ${CSPRO_DIR}/zIssaLib)

add_library(${PROJECT_NAME}

    # CEXEntry
    ${CEXENTRY_DIR}/Common.cpp
    ${CEXENTRY_DIR}/DeWrite.cpp
    ${CEXENTRY_DIR}/DicX.cpp
    ${CEXENTRY_DIR}/Entifaz.cpp
    ${CEXENTRY_DIR}/Relation.cpp
    ${CEXENTRY_DIR}/SecX.cpp
    ${CEXENTRY_DIR}/VarX.cpp

    # dep
    Intdentr.cpp
    intenter.cpp

    # ex2
    CIterator.cpp
    engdrv.cpp
    EngineDictionaryModifier.cpp
    Exappl.cpp
    Exapplva.cpp
    Exopfile.cpp
    GpsFuncs.cpp
    Int_set.cpp
    intcapi.cpp
    IntCase.cpp
    Intchars.cpp
    IntDataAccess.cpp
    Intdrive.cpp
    InterpreterAccessor.cpp
    Intexpr.cpp
    Intextdi.cpp
    IntFreq.cpp
    Intfuncs.cpp
    IntImpute.cpp
    Intinstr.cpp
    IntItem.cpp
    IntLanguage.cpp
    IntOccurrences.cpp
    IntPre77Report.cpp
    IntProgramControl.cpp
    IntProperties.cpp
    IntUserFunction.cpp
    IntUserInterface.cpp
    Listing.cpp
    LoadSaveBinary.cpp
    Nextnode.cpp
    ParadataDriver.cpp
    Pre80Routines.cpp

    # issa
    Applload.cpp
    Form.cpp
    Issafunc.cpp
    Messages.cpp
    RelT.cpp
    Settings.cpp
    Tables.cpp
    Tablesen.cpp
    VariableInOut.cpp

    # zCapiO
    ${ZCAPIO_DIR}/CapiCondition.cpp
    ${ZCAPIO_DIR}/CapiContentVirtualFileMapping.cpp
    ${ZCAPIO_DIR}/CapiQuestion.cpp
    ${ZCAPIO_DIR}/CapiQuestionManager.cpp
    ${ZCAPIO_DIR}/CapiQuestionYaml.cpp
    ${ZCAPIO_DIR}/CapiStyle.cpp
    ${ZCAPIO_DIR}/CapiText.cpp

    # zEntryO
    ${ZENTRYO_DIR}/CapiHandler.cpp
    ${ZENTRYO_DIR}/CaseTreeBuilder.cpp
    ${ZENTRYO_DIR}/CaseTreeNode.cpp
    ${ZENTRYO_DIR}/CaseTreeUpdate.cpp
    ${ZENTRYO_DIR}/CoreEntryEngineInterface.cpp
    ${ZENTRYO_DIR}/CoreEntryPage.cpp
    ${ZENTRYO_DIR}/CoreEntryPageField.cpp
    ${ZENTRYO_DIR}/DeploymentPackageDownloader.cpp
    ${ZENTRYO_DIR}/EntDrv.cpp
    ${ZENTRYO_DIR}/LevelNodeProcessing.cpp
    ${ZENTRYO_DIR}/RunAple.cpp
    ${ZENTRYO_DIR}/WEXEntry.cpp

    # zIssaLib
    ${ZISSALIB_DIR}/Attr.cpp
    ${ZISSALIB_DIR}/CFlAdmin.cpp
    ${ZISSALIB_DIR}/CFlow.cpp
    ${ZISSALIB_DIR}/CsDriver.cpp
    ${ZISSALIB_DIR}/CsDriver1.cpp
    ${ZISSALIB_DIR}/CsKernel.cpp
    ${ZISSALIB_DIR}/Defuncs.cpp
    ${ZISSALIB_DIR}/DicT.cpp
    ${ZISSALIB_DIR}/Dictload.cpp
    ${ZISSALIB_DIR}/EngineAccessor.cpp
    ${ZISSALIB_DIR}/EngineLoader.cpp
    ${ZISSALIB_DIR}/Exallocd.cpp
    ${ZISSALIB_DIR}/Exapplfu.cpp
    ${ZISSALIB_DIR}/Exapplin.cpp
    ${ZISSALIB_DIR}/ExNotes.cpp
    ${ZISSALIB_DIR}/FlowCore.cpp
    ${ZISSALIB_DIR}/FlowLoad.cpp
    ${ZISSALIB_DIR}/Form2.cpp
    ${ZISSALIB_DIR}/GroupT.cpp
    ${ZISSALIB_DIR}/GroupT2.cpp
    ${ZISSALIB_DIR}/GroupVisitor.cpp
    ${ZISSALIB_DIR}/Labsalo.cpp
    ${ZISSALIB_DIR}/OccurrenceInfoSet.cpp
    ${ZISSALIB_DIR}/OccurrenceVisitor.cpp
    ${ZISSALIB_DIR}/SecT.cpp
    ${ZISSALIB_DIR}/VarT.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    # yaml-cpp
    ${THIRD_PARTY_PREBUILT_LIB_DIR}/libyaml-cpp${THIRD_PARTY_LIB_EXT}
)
