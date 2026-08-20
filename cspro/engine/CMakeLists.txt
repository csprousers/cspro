project(engine)

set(CEXENTRY_DIRECTORY ${CSPRO_DIRECTORY}/engine)
set(ZCAPIO_DIRECTORY ${CSPRO_DIRECTORY}/zCapiO)
set(ZENTRYO_DIRECTORY ${CSPRO_DIRECTORY}/zEntryO)
set(ZISSALIB_DIRECTORY ${CSPRO_DIRECTORY}/zIssaLib)

include_directories(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIRECTORY}
    ${EXTERNAL_DIRECTORY}/geometry.hpp/include
    ${EXTERNAL_DIRECTORY}/variant/include
    ${EXTERNAL_DIRECTORY}/yaml-cpp/include
)

add_library(${PROJECT_NAME}

    # CEXEntry
    ${CEXENTRY_DIRECTORY}/Common.cpp
    ${CEXENTRY_DIRECTORY}/DeWrite.cpp
    ${CEXENTRY_DIRECTORY}/DicX.cpp
    ${CEXENTRY_DIRECTORY}/Entifaz.cpp
    ${CEXENTRY_DIRECTORY}/Relation.cpp
    ${CEXENTRY_DIRECTORY}/SecX.cpp
    ${CEXENTRY_DIRECTORY}/VarX.cpp

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
    IntMessages.cpp
    IntOccurrences.cpp
    IntPre77Report.cpp
    IntProgramControl.cpp
    IntProperties.cpp
    IntQuery.cpp
    IntSync.cpp
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
    ${ZCAPIO_DIRECTORY}/CapiCondition.cpp
    ${ZCAPIO_DIRECTORY}/CapiContentVirtualFileMapping.cpp
    ${ZCAPIO_DIRECTORY}/CapiQuestion.cpp
    ${ZCAPIO_DIRECTORY}/CapiQuestionManager.cpp
    ${ZCAPIO_DIRECTORY}/CapiQuestionYaml.cpp
    ${ZCAPIO_DIRECTORY}/CapiStyle.cpp
    ${ZCAPIO_DIRECTORY}/CapiText.cpp

    # zEntryO
    ${ZENTRYO_DIRECTORY}/CapiHandler.cpp
    ${ZENTRYO_DIRECTORY}/CaseTreeBuilder.cpp
    ${ZENTRYO_DIRECTORY}/CaseTreeNode.cpp
    ${ZENTRYO_DIRECTORY}/CaseTreeUpdate.cpp
    ${ZENTRYO_DIRECTORY}/CoreEntryEngineInterface.cpp
    ${ZENTRYO_DIRECTORY}/CoreEntryPage.cpp
    ${ZENTRYO_DIRECTORY}/CoreEntryPageField.cpp
    ${ZENTRYO_DIRECTORY}/DeploymentPackageDownloader.cpp
    ${ZENTRYO_DIRECTORY}/EntDrv.cpp
    ${ZENTRYO_DIRECTORY}/LevelNodeProcessing.cpp
    ${ZENTRYO_DIRECTORY}/RunAple.cpp
    ${ZENTRYO_DIRECTORY}/WEXEntry.cpp

    # zIssaLib
    ${ZISSALIB_DIRECTORY}/Attr.cpp
    ${ZISSALIB_DIRECTORY}/CFlAdmin.cpp
    ${ZISSALIB_DIRECTORY}/CFlow.cpp
    ${ZISSALIB_DIRECTORY}/CsDriver.cpp
    ${ZISSALIB_DIRECTORY}/CsDriver1.cpp
    ${ZISSALIB_DIRECTORY}/CsKernel.cpp
    ${ZISSALIB_DIRECTORY}/Defuncs.cpp
    ${ZISSALIB_DIRECTORY}/DicT.cpp
    ${ZISSALIB_DIRECTORY}/Dictload.cpp
    ${ZISSALIB_DIRECTORY}/EngineAccessor.cpp
    ${ZISSALIB_DIRECTORY}/EngineLoader.cpp
    ${ZISSALIB_DIRECTORY}/Exallocd.cpp
    ${ZISSALIB_DIRECTORY}/Exapplfu.cpp
    ${ZISSALIB_DIRECTORY}/Exapplin.cpp
    ${ZISSALIB_DIRECTORY}/ExNotes.cpp
    ${ZISSALIB_DIRECTORY}/FlowCore.cpp
    ${ZISSALIB_DIRECTORY}/FlowLoad.cpp
    ${ZISSALIB_DIRECTORY}/Form2.cpp
    ${ZISSALIB_DIRECTORY}/GroupT.cpp
    ${ZISSALIB_DIRECTORY}/GroupT2.cpp
    ${ZISSALIB_DIRECTORY}/GroupVisitor.cpp
    ${ZISSALIB_DIRECTORY}/Labsalo.cpp
    ${ZISSALIB_DIRECTORY}/OccurrenceInfoSet.cpp
    ${ZISSALIB_DIRECTORY}/OccurrenceVisitor.cpp
    ${ZISSALIB_DIRECTORY}/SecT.cpp
    ${ZISSALIB_DIRECTORY}/VarT.cpp
)
