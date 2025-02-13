
function fullLabel(toLabel, languages){//toLabel=readDict
    let fullLabels = "";
    let isLangs = false;
    if(toLabel.labels[0]){
        if(toLabel.labels[0].language){
            isLangs=true;
        }
    }
    let gapMarks = 0;       //number of | to place in between. Do not place at end.
    if (languages && isLangs){//needs set of languages, needs matching language tags.
        for(let l=0; l<Object.keys(languages).length; l++){         //for each language in the set
            let langName = languages[l].name;
            for(let ls=0; ls<toLabel.labels.length; ls++){          //for each language in labels, match
                //check for label matching language
                if (toLabel.labels[ls].language === langName){
                    if(l>0){
                        for (let i=0; i<gapMarks+1;i++){                //way to change number of gaps for languages in between
                            fullLabels += "|";
                        }
                        gapMarks = 0;
                    }
                    fullLabels += toLabel.labels[ls].text;
                    break;
                }
                else if (ls==toLabel.labels.length-1){              //language in set has no matches in labels
                    gapMarks += 1;
                }
            }
        }
    }
    else{
        for(let ls=0; ls<toLabel.labels.length; ls++){          //formerly the full function, but does not preserve order.
            if(ls>0){
                fullLabels += "|"
            }
            let badLabelRegex = /~|\|/g;
            fullLabels += toLabel.labels[ls].text.replaceAll(badLabelRegex," ");
        }
    }
    return fullLabels;
}

function fullValueSet(item, languages){
    //for each valueset in valuesets[index], if valuesets are present for item.
    let writeDict = [];

    if (item.valueSets){
        for(let valSet=0; valSet<item.valueSets.length; valSet++){//possibly multiple value sets within
            let vs = item.valueSets[valSet];
            writeDict.push("\n[ValueSet]");
            writeDict.push("Label=" + ( fullLabel(vs, languages) ));
            writeDict.push("Name=" + ( vs.name ));
            if (vs.link) {writeDict.push("Link=" + ( vs.link ));}
            if (vs.note){
                writeDict.push.apply(writeDict, takeNotes(vs));
            }
            //for each value in valueset, if present
            if(vs.values){
            for (let val=0; val<vs.values.length; val++){
                let v = vs.values[val];
                let valueStringTotal = "";
                for(let p=0; p<v.pairs.length; p++){
                    let valueString = "";

                    if(v.pairs[p].value){valueString += (v.pairs[p].value) };
                    if(v.pairs[p].range){valueString += (v.pairs[p].range[0] + ":" + v.pairs[p].range[1])};//alternative, range

                    //regex
                    let isnum = /^\d+$|^-\d+$/.test(valueString);
                    if ((item.contentType=="alpha" && (!isnum || valueString.length != item.length)) || (valueString.length==0)){
                        //SKIP case where valuestring is number of correct length
                        //stringify if type alpha and not number/bad length,
                        valueString = "'" + valueString;//prepend
                        while(valueString.length <= (item.length)){
                            valueString += " ";
                        }
                        valueString += "'";
                    }

                    if(p>0){valueStringTotal += (" ")};//spacing for reading
                    valueStringTotal += valueString;
                }

                if (fullLabel(v, languages).length > 0){valueStringTotal += ";" + fullLabel(v, languages);}
                writeDict.push("Value=" +  valueStringTotal);

                if(v.image){
                    let reverseSlash = v.image;
                    reverseSlash = reverseSlash.replaceAll("/","\\");
                    writeDict.push("Image=" + reverseSlash);
                }
                if(v.special=="MISSING"){
                    writeDict.push("Name=MISSING,Special");
                }
                if(v.special=="NOTAPPL"){
                    writeDict.push("Name=NOTAPPL,Special");
                }
                if(v.special=="DEFAULT"){
                    writeDict.push("Name=DEFAULT,Special");
                }
                if(v.special=="REFUSED"){
                    writeDict.push("Name=REFUSED,Special");
                }
                if(v.note){
                    writeDict.push.apply(writeDict, takeNotes(v));
                }
            }
            }
        }
    }
    return writeDict;
}

function takeNotes(item){
    let notesDict = [];
    let noteArray = item.note.split("\n");
    for (let n=0; n< noteArray.length; n++){
        if(n != (noteArray.length-1)){
            notesDict.push("Note=" + noteArray[n] + "\\r\\n");
        }
        else{
            notesDict.push("Note=" + noteArray[n]);
        }
    }
    return notesDict;
}

export function convertDictionary(dictionary, rootDirectory) {

    if( typeof dictionary === "string" ) {
        dictionary = JSON.stringify(dictionary);
    }

    let readDict = dictionary;
    const writeDict = [];
    //actual conversion of data
    // add this to some array of lines to be written
    if(readDict.editable==false){writeDict.push("[NoEdit]");}
    writeDict.push("[Dictionary]");
    writeDict.push("Version=CSPro 7.7");
    writeDict.push("Label=" + ( fullLabel(readDict, readDict.languages) ));
    writeDict.push("Name=" + ( readDict.name ));
    //if a note is present
    if (readDict.note){
        writeDict.push.apply(writeDict, takeNotes(readDict));
    }
    writeDict.push("RecordTypeStart=" + ( readDict.recordType.start ));
    writeDict.push("RecordTypeLen=" + ( readDict.recordType.length ));
    writeDict.push("Positions=" + ( readDict.relativePositions ? "Relative" : "Absolute" ));
    writeDict.push("ZeroFill=" + ( readDict.defaults.zeroFill ? "Yes" : "No" ));
    writeDict.push("DecimalChar=" + ( readDict.defaults.decimalMark ? "Yes" : "No" ));
    writeDict.push("SecurityOptions=" + ( readDict.security.settings.toUpperCase() ));

    //if Language(s) are present
    if (readDict.languages){
        writeDict.push("\n[Languages]");
        for (let langs=0; langs < readDict.languages.length; langs++){//auto-index through them
            writeDict.push(readDict.languages[langs].name + "=" + readDict.languages[langs].label);
        }
    }

    //for each level in level[index]
    let idStart = 0;
    let recStart = readDict.recordType.start + readDict.recordType.length;
    for (let level=0; level<readDict.levels.length; level++){
        let l=readDict.levels[level];
        for(let id=0; id<l.ids.items.length; id++){
            let item = l.ids.items[id];
            idStart = item.start + item.length;
        }
    }
    //max of: [id item start + id item length] or [record type start + record type length]
    let afterIDstart = Math.max(idStart, recStart);


    for (let level=0; level<readDict.levels.length; level++){
        let l=readDict.levels[level];
        writeDict.push("\n[Level]");
        writeDict.push("Label=" + ( fullLabel(l, readDict.languages) ));
        writeDict.push("Name=" + ( l.name ));

        writeDict.push("\n[IdItems]");

        //for each item in ids.items[index]
        for(let id=0; id<l.ids.items.length; id++){
            let item = l.ids.items[id];
            writeDict.push("\n[Item]");
            writeDict.push("Label=" + ( fullLabel(item, readDict.languages) ));
            writeDict.push("Name=" + ( item.name ));
            //if a note is present
            if(item.note){
                writeDict.push.apply(writeDict, takeNotes(item));
            }
            writeDict.push("Start=" + ( item.start ));
            writeDict.push("Len=" + ( item.length ));
            writeDict.push("DataType=" + ( item.contentType.charAt(0).toUpperCase() + item.contentType.slice(1) ));
            if (item.zeroFill){writeDict.push("ZeroFill=" + ( item.zeroFill ? "Yes" : "No" ))};
            writeDict.push.apply(writeDict, fullValueSet(item, readDict.languages));
        }

        //for each record in records[index]
        for(let record=0; record<l.records.length; record++){
            let curStart = afterIDstart;
            let rec = l.records[record];
            writeDict.push("\n[Record]");
            writeDict.push("Label=" + ( fullLabel(rec, readDict.languages) ));
            writeDict.push("Name=" + ( rec.name ));
            if (rec.note){
                writeDict.push.apply(writeDict, takeNotes(rec));
            }
            if (rec.recordType) {
                writeDict.push("RecordTypeValue=" + ( "'" + rec.recordType + "'" ));
            }
            else{
                writeDict.push("RecordTypeValue=" + ( "''" ));
            }
            if(!rec.occurrences.required){writeDict.push("Required=" + ( rec.occurrences.required ? "Yes" : "No" ))};
            if(rec.occurrences.maximum>1){
                writeDict.push("MaxRecords=" + ( rec.occurrences.maximum ));
            }
            writeDict.push("RecordLen=unknown");
            if(rec.occurrences){
                if(rec.occurrences.labels){
                    for (let occ=0; occ < rec.occurrences.labels.length; occ++) {
                        let curOcc = rec.occurrences.labels[occ];
                        if(curOcc.labels){
                            writeDict.push("OccurrenceLabel=" + curOcc.occurrence + "," + fullLabel(curOcc, readDict.languages));
                        }
                        else{
                            writeDict.push("OccurrenceLabel=" + curOcc.occurrence);
                        }
                    }
                }
            }
            //for each item in items[index]

            for(let id=0; id<rec.items.length; id++){
                let item = rec.items[id];
                writeDict.push("\n[Item]");
                writeDict.push("Label=" + ( fullLabel(item, readDict.languages) ));
                writeDict.push("Name=" + ( item.name ));
                //if a note is present
                if(item.note){
                    writeDict.push.apply(writeDict, takeNotes(item));
                }
                if(item.subitem==true){
                    if(item.start){ //absolute positioning
                        writeDict.push("Start=" + item.start);
                    }
                    else{
                        writeDict.push("Start=" + ( curStart + item.subitemOffset ));
                    }
                    writeDict.push("Len=" + ( item.length ));
                    writeDict.push("ItemType=SubItem");
                    //start is actually at start of non-subitem record item + subitem offset
                }
                else{
                    if(id>0){//add length of last item (x-1) to curstart. Doesn't go on first item.
                        for(let x=1; x<=id; x++){
                            if(!rec.items[id-x].subitem){
                                //id-x is previous full item
                                if(rec.items[id-x].occurrences){
                                    //previous full item had occurrences
                                    curStart = curStart + rec.items[id-x].occurrences.maximum * rec.items[id-x].length;
                                    break;
                                }
                                else{
                                    //previous full item had no occurrences
                                    curStart = curStart + rec.items[id-x].length;
                                    break;
                                }
                            }
                        }
                    }
                    if(item.start){ //absolute positioning
                        writeDict.push("Start=" + ( item.start ));
                    }
                    else{
                        writeDict.push("Start=" + ( curStart ));
                    }
                    writeDict.push("Len=" + ( item.length ));
                }
                writeDict.push("DataType=" + ( item.contentType.charAt(0).toUpperCase() + item.contentType.slice(1) ));
                if(item.occurrences){
                    writeDict.push("Occurrences=" + item.occurrences.maximum );
                }
                if(item.decimals){
                    writeDict.push("Decimal=" + ( item.decimals ));
                    writeDict.push("DecimalChar=" + ( item.decimalMark ? "Yes" : "No" ));
                    //only if yes?
                }
                if (item.zeroFill){writeDict.push("ZeroFill=" + ( item.zeroFill ? "Yes" : "No" ))};

                if(item.capture){
                    if(item.capture.captureType){writeDict.push("CaptureType=" + ( item.capture.captureType ));}
                    if(item.capture.dateCaptureInfo){writeDict.push("CaptureDateFormat=" + ( item.capture.dateCaptureInfo ));}
                }

                if(item.occurrences){
                    if(item.occurrences.labels){
                        for (let occ=0; occ < item.occurrences.labels.length; occ++) {
                            let curOcc = item.occurrences.labels[occ];
                            if(curOcc.labels){
                                writeDict.push("OccurrenceLabel=" + curOcc.occurrence + "," + fullLabel(curOcc, readDict.languages));
                            }
                            else{
                                writeDict.push("OccurrenceLabel=" + curOcc.occurrence);
                            }
                        }
                    }
                }
                writeDict.push.apply(writeDict, fullValueSet(item, readDict.languages));

            }
            //record over, go back and place length, catch last item
            let lastIndex = rec.items.length-1
            if(rec.items[lastIndex]){
            while(rec.items[lastIndex].subitem){
                lastIndex = lastIndex-1;
            }
            if(rec.items[lastIndex].occurrences){
                //last full item had occurrences
                curStart = curStart + rec.items[lastIndex].occurrences.maximum * rec.items[lastIndex].length;
            }
            else{
                //last full item had no occurrences
                curStart = curStart + rec.items[lastIndex].length;
            }
            }
            let rewriteIndex = writeDict.indexOf("RecordLen=unknown");
            if(readDict.relativePositions==false){
                let maxEnd = afterIDstart;
                for (let a=0; a<rec.items.length; a++){
                    let newEnd = rec.items[a].start + rec.items[a].length;
                    if (newEnd >= maxEnd){
                        maxEnd = newEnd;
                    }
                }
                if(maxEnd>0){maxEnd-=1;}
                writeDict[rewriteIndex] = ("RecordLen=" + maxEnd);
            }
            else{
                writeDict[rewriteIndex] = ("RecordLen=" + (curStart-1));
            }
        }
    }

    //for each relation in relations[index] if present
    if(readDict.relations){
        for (let relation=0; relation<readDict.relations.length; relation++){
            let rel=readDict.relations[relation];
            writeDict.push("\n[Relation]");
            writeDict.push("Name=" + ( rel.name ));
            writeDict.push("Primary=" + ( rel.primary ));
            //for link in links[index]
            for (let link=0; link<rel.links.length; link++){
                let lin=rel.links[link];
                if (lin.primaryLink != "(occurrence)"){writeDict.push("PrimaryLink=" + ( lin.primaryLink ));}
                if (lin.secondary != "(occurrence)"){writeDict.push("Secondary=" + ( lin.secondary ));}
                if (lin.secondaryLink != "(occurrence)"){writeDict.push("SecondaryLink=" + ( lin.secondaryLink ));}
            }
        }
    }
    return writeDict.join("\n") + "\n";
}
