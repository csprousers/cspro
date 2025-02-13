var _ip;
class CaseViewInputProcessor {
    testField = "test field";
    result = {};
    builders = {
        "field": this.buildField,
        "roster": this.buildRoster,
        "block": this.buildBlock
    }

    rawInput = null;
    itemsByName = {};
    items = [];
    qsfByName = {};
    itemAddr = {};
    itemAddrObj = {};
    subitemParents = {};
    notesByName = {};

    factory = null;

    constructor(factory) {
        this.factory = factory;
    }

    indexDictItems() {
        this.itemsByName = {};
        this.dictItems = [];
        let dict = this.rawInput["dictionary"];
        if (!dict)
            return;

        var dName = dict.name;

        if (!dict["levels"])
            return;

        var _this = this;
        dict.levels.forEach(lev => {
            //id record
            var curItem = null;
            lev.ids.items.forEach(i => {
                let iName = `${dName}.${i.name}`;
                _this.itemsByName[iName] = i;
                _this.items.push(i);

                _this.itemAddr[iName] = [lev.name, i.name];
                _this.itemAddrObj[iName] = [lev, i];

                //indexing parent items for subitems
                if (i.subitem === true) {
                    _this.subitemParents[iName] = curItem;
                } else {
                    curItem = i;
                }
            });

            //other records
            lev.records.forEach(r => {
                curItem = null;
                r.items.forEach(i => {
                    let iName = `${dName}.${i.name}`;
                    _this.itemsByName[iName] = i;
                    _this.items.push(i);

                    _this.itemAddr[iName] = [lev.name, r.name, i.name];
                    _this.itemAddrObj[iName] = [lev, r, i];

                    //indexing parent items for subitems
                    if (i.subitem === true) {
                        _this.subitemParents[iName] = curItem;
                    } else {
                        curItem = i;
                    }
                });
            });
        });
    }

    indexQsf() {
        this.qsfByName = {};
        let qt = this.rawInput["questionText"];
        if (!qt)
            return;

        if (!qt["questions"])
            return;

        var _this = this;
        qt.questions.forEach(q => {
            _this.qsfByName[`${q["dictionary"] ? q.dictionary : "block"}.${q.name}`] = q;
        });
    }

    indexNotes() {
        this.notesByName = {};
        let caseData = this.rawInput["case"];
        if (!caseData)
            return;

        if (!caseData["notes"])
            return;

        var _this = this;
        caseData.notes.forEach(q => {
            _this.notesByName[`${q.name}.occ${q["occurrences"] ? q.occurrences.record : 0}`] = q;
        });
    }

    setCaptureCaps() {
        this.items.forEach(i => {
            //check box
            i["canBeCheckBox"] = false;
            if (i.contentType === "alpha" && i.valueSets && i.valueSets.length > 0) {
                if (i.valueSets[0].values && i.valueSets[0].values.length > 0) {
                    let firstValLen = i.valueSets[0].values[0].pairs[0].value.length;
                    let canBeCB = true;
                    for (var j = 1; j < i.valueSets[0].values.length; j++) {
                        if (firstValLen != i.valueSets[0].values[j].pairs[0].value.length) {
                            canBeCB = false;
                            j = i.valueSets[0].values.length;
                        }
                    }
                    i.canBeCheckBox = canBeCB;
                    if (canBeCB)
                        i["checkBoxValLen"] = firstValLen;
                }
            }
        });
    }

    convertInput(inputJson) {
        _ip = this;

        this.rawInput = inputJson;
        this.result = {};

        this.indexDictItems();
        this.indexQsf();
        this.indexNotes();

        //setting capture capabilities for all items
        this.setCaptureCaps();

        this.result["label"] = "";
        if (inputJson["forms"]) {
            this.result.label = inputJson.forms["label"];
        }

        //languages
        this.extractLanguages(inputJson);
        this.updateLanguagesPerQsf(inputJson);
        this.result["multLang"] = (this.result.languages.length > 1);

        //forms
        this.extractForms(inputJson);

        //QSF styles
        this.extractStyles(inputJson);

        this.result["noCase"] = !inputJson["case"];
    }

    extractLanguages(inputJson) {
        this.result["languages"] = [];
        this.result["langIds"] = [];
        this.result["langDict"] = {};
        let dict = inputJson["dictionary"];

        if (!dict || !dict["languages"]) {
            this.result.languages.push("English");
            this.result.langIds.push("EN");
            this.result.langDict["EN"] = "English";
            return;
        }

        dict.languages.forEach(lang => {
            this.result.languages.push(lang["label"]);
            this.result.langIds.push(lang["name"]);
            this.result.langDict[lang["name"]] = lang["label"];
        });
    }

    updateLanguagesPerQsf(inputJson) {
        let qsf = inputJson["questionText"];

        if (!qsf || !qsf["languages"])
            return;

        var _this = this;
        qsf.languages.forEach(lang => {
            if (!_this.result.langDict[lang["name"]]) {
                this.result.languages.push(lang["label"]);
                this.result.langIds.push(lang["name"]);
                this.result.langDict[lang["name"]] = lang["label"];
            }
        });
    }

    extractForms(inputJson) {
        this.result["forms"] = [];

        let form = inputJson["forms"];
        if (!form || !form["levels"]) {
            return;
        }

        var _this = this;
        form.levels.forEach(lev => {
            if (lev["items"]) {
                lev.items.forEach(ff => {
                    _this.extractForm(ff);
                });
            }
        });
    }

    extractStyles(inputJson) {
        this.result["styles"] = [];

        if (inputJson["questionText"] && inputJson.questionText["styles"])
            this.result.styles = inputJson.questionText.styles;
    }

    extractForm(form) {
        var f = {};

        f["name"] = form["name"];
        f["label"] = form["label"];
        f["elements"] = [];

        //building elements
        this.buildFormElements(form["items"], f.elements);


        this.result.forms.push(f);
    }

    buildFormElements(items, elements) {
        var _this = this;
        items.forEach(item => {
            elements.push(_this.builders[item.type](item));
        });
    }

    findDictItem(dictName, itemName) {
        var iName = `${dictName}.${itemName}`;
        return this.itemsByName[iName];
    }

    getQsfText(dictName, itemName) {
        var res = {};
        var iName = `${dictName}.${itemName}`;
        var qsf = this.qsfByName[iName];
        if (!qsf)
            return res;

        if (!qsf["conditions"])
            return res;

        if (!qsf.conditions[0]["texts"])
            return res;

        qsf.conditions[0].texts.forEach(t => {
            res[t.language] = t.html.replace(/~~/g, '');
            //res.push(t.html.replace(/~~/g, ''));
        });

        return res;
    }

    getValue(dictName, itemName, occ) {
        var res = this.rawInput["case"];
        if (!res)
            return null;

        var iName = `${dictName}.${itemName}`;
        var addr = this.itemAddr[iName];

        if (addr) {
            var addrIdx = 0;
            addr.forEach(a => {
                if (!res[a])
                    return null;

                res = res[a];
                if (Array.isArray(res)) {
                    if (_ip.caseMaxOcc < res.length)
                        _ip.caseMaxOcc = res.length;
                    // if (res.length == 1 && addrIdx < addr.length - 1)
                    //     res = res[0];
                    // else
                    if (occ < res.length)
                        res = res[occ];
                    else
                        return null;
                }

                addrIdx++;
            });
        }

        return res;
    }

    //delimiting value by single dictionary value length
    delimitValue(val, vLen) {
        var res = [];
        if (val.length && val.length > 0) {
            if (vLen) {
                for (let i = 0; i < val.length; i += vLen) {
                    if (i + vLen <= val.length) {
                        res.push(val.substring(i, i + vLen));
                    }
                }
            }
        }
        return res;
    }

    valueInRange(dictItem, from, to, val, isCheckBox) {
        var res = false;
        if (dictItem.contentType == "numeric") {
            let v = parseFloat(val);
            if (!isNaN(v)) {
                res = v >= parseFloat(from) && v <= parseFloat(to);
            } else {
                res = val.trim() == from.trim();
            }
        } else {
            if (isCheckBox) {
                res = this.delimitValue(val.trim(), dictItem.checkBoxValLen).includes(from.trim());
            } else {
                res = val.trim() == from.trim();
            }
        }

        return res;
    }

    buildValue(value, dictItem, vv, captureType) {

        var labels = [];
        value.labels.forEach(l => {
            labels.push(l.text);
        });

        let resArr = [];

        var imageUrl = null;
        if (value["url"]) {
            imageUrl = value.url;
        }

        value.pairs.forEach(vals => {
            var res = {
                label: labels,
                value: ""
            }

            var from = null;
            var to = null;
            var isRange = false;
            if ("value" in vals) { //single value
                res.value = vals.value;
                from = vals.value;
                to = vals.value;
            } else { //range
                isRange = true;
                from = vals.range[0];
                to = vals.range[1];
                res.value = {
                    type: "textbox",
                    value: "",
                    from: from,
                    to: to,
                    size: dictItem.length,
                    alignRight: dictItem.contentType == "numeric",
                    floatRight: true
                };
            }

            //setting checked flag
            var isCheckBox = (captureType == "ct-check-box" && dictItem.canBeCheckBox === true);

            if (!vv)
                vv = {};
            if (!vv.code)
                vv.code = "";
            if (this.valueInRange(dictItem, from, to, vv.code, isCheckBox)) {
                res["checked"] = "checked";
                if (isRange) {
                    res.value.value = vv.code;
                }
            }

            //setting image
            if (imageUrl != null) {
                res.image = imageUrl;
            }

            resArr.push(res);
        });

        return resArr;
    }

    buildValueSet(item, vs, dictItem, vv, captureType) {
        var ct = captureType;
        var res = {
            name: vs.name,
            label: [],
            dcfLabel: [],
            captureType: ct,
            values: []
        };

        vs.labels.forEach(l => {
            res.label.push(l.text);
            res.dcfLabel.push(l.text);
        });

        vs.values.forEach(v => {
            //res.values.push(_ip.buildValue(v, dictItem, vv));
            res.values = res.values.concat(_ip.buildValue(v, dictItem, vv, captureType));
        });

        return res;
    }

    getNote(item, occ) {
        let noteName = `${item.name}.occ${occ ? occ : 0}`;
        var note = this.notesByName[noteName];

        if (!note)
            return null;

        let t = item.name;
        if (occ)
            t += `(${occ})`;

        return {
            title: t,
            text: note.text.replace(/\\n/g, '<br>')
        };
    }

    buildField(item, occ) {
        if (!occ)
            occ = 0;

        //accounting for mirror fields
        if (item.mirror) {
            item.name = item.name.substring(0, item.name.length - 3);
        }

        //finding dictionary item
        var i = _ip.findDictItem(item.dictionary, item.name);

        var res = {
            "type": "item",
            "label": "",
            "qsfText": "",
            "captureType": "ct-text-box",
            "showTextBox": true,
            "showValueSets": false,
            "value": {
                "type": "textbox",
                "value": "",
                "from": "",
                "to": "",
                "size": 1,
                "alignRight": false
            },
            "valueSets": []
        };

        //label
        res["dcfLabel"] = [];
        if (i) {
            i.labels.forEach(l => {
                res.dcfLabel.push(`${item.name}: ${l.text}`);
            });
        }

        //qsf
        res["qsfText"] = _ip.getQsfText(item.dictionary, item.name);

        //size
        res.value.size = i.length;

        //value
        var v = _ip.getValue(item.dictionary, item.name, occ);
        if (v && v.code) {
            res.value.value = v.code;
        }

        //alignment
        if (i.contentType == "numeric") {
            res.value.alignRight = true;
        }

        //note
        var note = _ip.getNote(item, occ);
        if (note != null)
            res["note"] = note;

        //capture type
        var ct = "";
        if (item["capture"] && item.capture["type"])
            ct = item.capture.type;


        if (ct == "radioButton" || ct == "comboBox") {
            res.showTextBox = false;
            res.showValueSets = true;
            res.captureType = "ct-radio-button";

            for (const vs of i.valueSets) {
                res.valueSets.push(_ip.buildValueSet(item, vs, i, v, res.captureType));
                if (_ip.factory.printAllValueSets === false) {
                    break;
                }
            }
        } else if (ct == "checkBox") {
            res.showTextBox = false;
            res.showValueSets = true;
            res.captureType = "ct-check-box";

            for (const vs of i.valueSets) {
                res.valueSets.push(_ip.buildValueSet(item, vs, i, v, res.captureType));
                if (_ip.factory.printAllValueSets === false) {
                    break;
                }
            }
        } else if (ct == "barcode") {
            res.captureType = "ct-barcode";
            res.value.type = "barcode";
        }

        return res;
    }

    buildRosterItem(res, i, rParam, blockData) {

        var i1 = {
            dcfLabel: [],
            qsfText: [],
            captureType: "ct-text-box"
        };

        var dictItem = _ip.findDictItem(i.dictionary, i.name);

        //finding item record
        let iName = `${i.dictionary}.${i.name}`;
        if (_ip.itemAddrObj[iName] && _ip.itemAddrObj[iName].length > 2) {
            rParam.rosterRec = _ip.itemAddrObj[iName][1];
            rParam.maxOcc = rParam.rosterRec.occurrences.maximum;
        }

        //if single item roster
        if (rParam.maxOcc === 1 && dictItem.occurrences.maximum > 1) {
            rParam.maxOcc = dictItem.occurrences.maximum;
        }

        //if subitem, checking if this is a group roster, or a single item roster
        if (rParam.maxOcc === 1 && dictItem.subitem === true && _ip.subitemParents[iName] != null) {
            rParam.maxOcc = _ip.subitemParents[iName].occurrences.maximum;
        }

        //label
        dictItem.labels.forEach(l => {
            i1.dcfLabel.push(`${i.name}: ${l.text}`);
        });

        //qsf
        i1.qsfText = _ip.getQsfText(i.dictionary, i.name);

        //adding block data
        if (blockData) {
            if (blockData.blockStart === true) {
                i1["blockStart"] = true;
                i1["block"] = blockData.block;
            } else if (blockData.blockEnd === true) {
                i1["blockEnd"] = true;
            }
        }

        res.horizontal.items.push(i1);
    }

    caseMaxOcc = 0;
    buildRoster(item) {
        var res = {};

        res["type"] = "roster";
        res["label"] = item.label;
        res["orientation"] = (item.orientation == "horizontal" ? "Horizontal" : "Vertical");
        res["showTransposeButton"] = true;

        res["horizontal"] = {
            "items": [],
            "occurrences": []
        }

        //items
        var rParam = {
            resterRec: null,
            maxOcc: 1
        };
        var blockStartItems = [];
        item.items.forEach(i => {
            if (i.type == "field")
                _ip.buildRosterItem(res, i, rParam);
            else if (i.type == "block") {
                var idx = 0;
                i.items.forEach(bi => {
                    //getting block data to be passed over to items
                    var blockData = null;
                    if (idx == 0) {
                        blockData = {
                            blockStart: true,
                            blockEnd: false
                        };

                        blockData["block"] = {
                            qsfText: _ip.getQsfText("block", i.name),
                            label: `${i.name}: ${i.label}`
                        };
                    } else if (idx == i.items.length - 1) {
                        blockData = {
                            blockStart: false,
                            blockEnd: true
                        };
                    }

                    _ip.buildRosterItem(res, bi, rParam, blockData);

                    //memoizing blockStart item to add colSpan later
                    if (blockData != null && (blockData.blockStart === true || blockData.blockEnd === true)) {
                        blockStartItems.push(res.horizontal.items.length - 1);
                    }

                    idx++;
                });
            }

        });

        res["maxOcc"] = rParam.maxOcc;

        //limiting max occurrences if needed
        let maxOcc = _ip.factory.maxRosterOcc;
        if (maxOcc > 0 && rParam.maxOcc > maxOcc) {
            rParam.maxOcc = maxOcc;
        }

        _ip.caseMaxOcc = 0;

        //occurrences
        for (var i = 0; i < rParam.maxOcc; i++) {
            var occ = {
                index: (i + 1),
                cells: []
            };

            item.items.forEach(it => {
                if (it.type == "field")
                    occ.cells.push(_ip.builders[it.type](it, i));
                else if (it.type == "block") {
                    it.items.forEach(bi => {
                        occ.cells.push(_ip.builders[bi.type](bi, i));
                    });
                }
            });

            if (_ip.rawInput.case)
                rParam.maxOcc = _ip.caseMaxOcc;

            if (i < rParam.maxOcc)
                res.horizontal.occurrences.push(occ);
        }

        if (rParam.maxOcc == 0) {
            res.orientation = "Horizontal";
            res.showTransposeButton = false;
        }

        //setting column span value for the block
        blockStartItems.forEach(i => {
            res.horizontal.items[i]["colSpan"] = rParam.maxOcc + 2;
        });

        return res;
    }

    buildBlock(item) {
        var res = {};

        res["type"] = "block";
        res["name"] = item.name;
        res["label"] = item.label;
        res["items"] = [];

        //qsf
        res["qsfText"] = _ip.getQsfText("block", item.name);

        if (item.items) {
            item.items.forEach(it => {
                res.items.push(_ip.builders[it.type](it));
            });
        }

        return res;

    }

}