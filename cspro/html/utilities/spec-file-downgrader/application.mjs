
function pathify(fileString){
    fileString = fileString.replaceAll("/","\\");
    if (fileString.charAt(0)=="."){
        return fileString;
    }
    else{
        return (".\\"+fileString);
    }
}

export function convertApplication(application, rootDirectory) {

    if( typeof application === "string" ) {
        application = JSON.stringify(application);
    }

    const writeApp = []
    if(application.editable==false){writeApp.push("[NoEdit]"); }
    writeApp.push("[CSPro Application]");
    writeApp.push("Version=CSPro 7.7");
    /*if label?*/writeApp.push("Label=" + ( application.label )); //only single label, not labels.
    writeApp.push("Name=" + ( application.name ));
    if(application.type == "entry"){
        writeApp.push("AppType=DataEntry");
    }
    else{
        writeApp.push("AppType=" + ( application.type.charAt(0).toUpperCase() + application.type.slice(1) ));
    }
    //convert Entry to DataEntry
    if(application.properties){
        if(application.properties.askOperatorId==false){writeApp.push("OperatorID=" + ( application.properties.askOperatorId ? "Yes" : "No" ));}

        if(application.properties.htmlDialogs==false){writeApp.push("UseHTMLDialogs=" + ( application.properties.htmlDialogs ? "Yes" : "No" ));}
        if(application.properties.showEndCaseMessage==false){writeApp.push("ShowEndCaseDialog=" + ( application.properties.showEndCaseMessage ? "Yes" : "No" ));}
        if(application.properties.partialSave){
            if(application.properties.partialSave.operatorEnabled==true){
                writeApp.push("PartialSave=" + ( application.properties.partialSave.operatorEnabled ? "Yes" : "No" ));
            }
            if(application.properties.partialSave.autoSaveMinutes>0){
                writeApp.push("AutoPartialSaveMinutes=" + application.properties.partialSave.autoSaveMinutes);
            }
        }
        if(application.properties.showQuestionText==true){writeApp.push("CAPI=Yes"); }
        if(application.properties.caseTree=="on"){
            writeApp.push("CaseTree=Always");
        }
        else if(application.properties.caseTree=="off"){
            writeApp.push("CaseTree=Never");
        }

        if(application.properties.centerForms==true){writeApp.push("CenterForms=" + ( application.properties.centerForms ? "Yes" : "No" ));}
        if(application.properties.decimalMark==false){writeApp.push("DecimalComma=" + ( application.properties.decimalMark ? "Yes" : "No" ));}
        if(application.properties.createListing==false){writeApp.push("CreateListing=" + ( application.properties.createListing ? "Yes" : "No" ));}
        if(application.properties.createLog==false){writeApp.push("CreateLog=" + ( application.properties.createLog ? "Yes" : "No" ));}
        if(application.properties.notes){
            if(application.properties.notes.delete_){writeApp.push("NotesDeleteOtherOperators=" + ( application.properties.notes.delete_ ? "Yes" : "No" ));}
            if(application.properties.notes.edit==false){writeApp.push("NotesEditOtherOperators=" + ( application.properties.notes.edit ? "Yes" : "No" ));}
        }
        if(application.properties.autoAdvanceOnSelection==true){writeApp.push("AutoAdvanceOnSelection=" + ( application.properties.autoAdvanceOnSelection ? "Yes" : "No" ));}
        if(application.properties.displayCodesAlongsideLabels==true){writeApp.push("DisplayCodesAlongsideLabels=" + ( application.properties.displayCodesAlongsideLabels ? "Yes" : "No" ));}
        if(application.properties.showFieldLabels==false){writeApp.push("ShowFieldLabels=" + ( application.properties.showFieldLabels ? "Yes" : "No" ));}
        if(application.properties.showErrorMessageNumbers==false){writeApp.push("ShowErrorMessageNumbers=" + ( application.properties.showErrorMessageNumbers ? "Yes" : "No" ));}
        if(application.properties.showOnlyDiscreteValuesInComboBoxes==true){writeApp.push("ComboBoxShowOnlyDiscreteValues=" + ( application.properties.showOnlyDiscreteValuesInComboBoxes ? "Yes" : "No" ));}
        if(application.properties.showRefusals==false){writeApp.push("ShowRefusals=" + ( application.properties.showRefusals ? "Yes" : "No" ));}
        //verify
        if(application.properties.verify){
            if(application.properties.verify.frequency!=1 || application.properties.verify.start!=1){
                writeApp.push("VerifyFrequency=" + ( application.properties.verify.frequency));
                if(application.properties.verify.start=="random"){
                    writeApp.push("VerifyStart=RANDOM");
                }
                else{
                    writeApp.push("VerifyStart=" + ( application.properties.verify.start));
                }
            }
        }
        if(application.properties.paradata){
            if(application.properties.paradata.collection=="all"){
                writeApp.push("ParadataCollection=AllEvents");
            }
            else if (application.properties.paradata.collection=="partial"){
                writeApp.push("ParadataCollection=SomeEvents");
            }
            if(application.properties.paradata.recordIteratorLoadCases==true){writeApp.push("RecordIteratorLoadCases=Yes")}
            if(application.properties.paradata.recordValues==true){writeApp.push("ParadataRecordValues=Yes")}
            if(application.properties.paradata.recordCoordinates==true){writeApp.push("ParadataRecordCoordinates=Yes")}
            if(application.properties.paradata.recordInitialPropertyValues==true){writeApp.push("RecordInitialPropertyValues=Yes")}
            if(application.properties.paradata.deviceStateIntervalMinutes){writeApp.push("ParadataDeviceStateMinutes=" + application.properties.paradata.deviceStateIntervalMinutes);}
            if(application.properties.paradata.gpsLocationIntervalMinutes){writeApp.push("ParadataGpsLocationMinutes=" + application.properties.paradata.gpsLocationIntervalMinutes);}
        }
    }
    if(application.dictionaries){
        let anyExternal = 0;
        for (let d=0; d<application.dictionaries.length; d++){
            let t = application.dictionaries[d];
            if (t.type=="external"|| t.type=="working" || t.type=="output"){
                if(t.parent){
                    if(t.parent.slice(-4)==".fmf"){//not external fmf files
                        continue;
                    }
                }
                if (anyExternal==0){
                    writeApp.push("[External Dictionaries]");   // in xtb. Just filename, not type/parent like [Dictionary Types]
                    anyExternal = 1;
                }
                writeApp.push("File=" + pathify(application.dictionaries[d].path));
            }
        }
    }
    if(application.code){
        writeApp.push("[AppCode]");                //in all
        for (let c=0; c<application.code.length; c++){
            if (application.code[c].type=="external"){
                writeApp.push("Include=" + pathify(application.code[c].path));
            }
            else{
                writeApp.push("File=" + pathify(application.code[c].path));
            }
        }
    }
    if(application.messages){
        writeApp.push("[Message]");                 //in all
        for (let c=0; c<application.messages.length; c++){
            if(c==0){
                writeApp.push("File=" + pathify(application.messages[c]));
            }
            else{
                writeApp.push("Include=" + pathify(application.messages[c]));
            }
        }
    }
    if(application.reports){
        writeApp.push("[Reports]");                 //in all
        for (let c=0; c<application.reports.length; c++){
            writeApp.push(application.reports[c].name.toUpperCase() + "=" + pathify(application.reports[c].path));
        }
    }

    if(application.questionText){
        writeApp.push("[Help]");                   //in ent
        for (let c=0; c<application.questionText.length; c++){
            writeApp.push("File=" + pathify(application.questionText[c]));
        }
    }

    if(application.forms){
        writeApp.push("[Forms]");                   //in ent
        for (let c=0; c<application.forms.length; c++){
            writeApp.push("File=" + pathify(application.forms[c]));
        }
    }
    if(application.order){
        writeApp.push("[Order]");                   //in bch
        for (let c=0; c<application.order.length; c++){
            writeApp.push("File=" + pathify(application.order[c]));
        }
    }
    if(application.tableSpecs){
        writeApp.push("[TabSpecs]");                //in xtb
        for (let c=0; c<application.tableSpecs.length; c++){
            writeApp.push("File=" + pathify(application.tableSpecs[c]));
        }
    }

    if(application.resources){
        writeApp.push("[Resources]");
        for(let r=0; r<application.resources.length; r++){
            writeApp.push("Folder=" + pathify(application.resources[r]));
        }
    }

    if(application.dictionaries.length>0){
        writeApp.push("[Dictionary Types]");        //in all
        //for (let c=0; c<application.dictionaries.length; c++){
        for (let c=0; c<application.dictionaries.length; c++){
            let dictTypeString = "Dict-Type=";
            dictTypeString += "" + pathify(application.dictionaries[c].path) + "," + application.dictionaries[c].type.charAt(0).toUpperCase() + application.dictionaries[c].type.slice(1);
            if(application.dictionaries[c].parent){dictTypeString += "," + pathify(application.dictionaries[c].parent);}
            writeApp.push(dictTypeString);
            //should automatically detect, only add .\ if there's no ".\", "..\", etc before filename
        }
    }
    if(application.properties.sync){
        writeApp.push("[Sync]");
        writeApp.push("Server=" + application.properties.sync.server);
        writeApp.push("Direction=" + application.properties.sync.direction.charAt(0).toUpperCase() + application.properties.sync.direction.slice(1));
    }
    if(application.properties.caseListing){
        if(application.properties.caseListing.type=="map"){
            writeApp.push("[Mapping]");
            writeApp.push("LatitudeItem=" + application.properties.caseListing.latitude);
            writeApp.push("LongitudeItem=" + application.properties.caseListing.longitude);
        }
    }

    return writeApp.join("\n") + "\n";
}
