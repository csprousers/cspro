//parent class for a form element
class FormElement {
    type = "";
    factory = {};
    constructor(type, factory) {
        this.type = type;
        this.factory = factory;
    }

    render(dataJson, container) {
        //incrementing element ID
        dataJson["id"] = ++this.factory.elementId;
    }

    preproc(dataJson) {
        //nothing to do here
    }

    postProc() {
        //nothing to do here
    }
}

class Item extends FormElement {
    constructor(factory) {
        super("item", factory);
    }

    render(dataJson, container) {
        var factory = this.factory;
        FormElement.prototype.render.call(this, dataJson, container);

        //setting labels per language
        //QSF text
        dataJson["qsfText-lang"] = factory.resolveLabelLanguage(dataJson.qsfText);
        dataJson["dcfLabel-lang"] = factory.resolveLabelLanguage(dataJson.dcfLabel);

        //Values
        if (dataJson.valueSets && dataJson.valueSets.length > 0) {
            dataJson.valueSets.forEach(function (vs) {
                if (vs.values.length > 0) {
                    vs.values.forEach(function (v) {
                        v["label-lang"] = factory.resolveLabelLanguage(v.label);
                        if (v.value) {
                            v["value"] = factory.renderValue(v.value);
                        }
                    });
                }
            });
        }

        //rendering textbox
        if (dataJson.showTextBox) {
            dataJson["value"] = factory.renderValue(dataJson.value);
        }

        var templateName = factory.getTemplateName(`${this.type}Template`);

        $(container).append(factory.templates[templateName](dataJson));


    }

    preproc(dataJson) {
        FormElement.prototype.preproc.call(this, dataJson);

        //nothing to do here
    }

    postProc() {
        FormElement.prototype.postProc.call(this);

        //nothing to do here
    }
}

class Block extends FormElement {
    constructor(factory) {
        super("block", factory);
    }

    render(dataJson, container) {
        var factory = this.factory;
        FormElement.prototype.render.call(this, dataJson, container);

        //QSF text
        dataJson["qsfText-lang"] = factory.resolveLabelLanguage(dataJson.qsfText);
        dataJson["label-lang"] = factory.resolveLabelLanguage(dataJson.label);

        var templateName = factory.getTemplateName(`${this.type}Template`);

        $(container).append(factory.templates[templateName](dataJson));

        //appending children
        var blockItemsContainer = $(`#blockItemsContainer${dataJson.id}`);
        dataJson.items.forEach(function (elementJson) {
            factory
                .formElementTypes[elementJson.type]
                .render(elementJson, blockItemsContainer[0]);
        });
    }

    preproc(dataJson) {
        FormElement.prototype.preproc.call(this, dataJson);

        //nothing to do here
    }

    postProc() {
        FormElement.prototype.postProc.call(this);

        //nothing to do here
    }
}

class Roster extends FormElement {
    constructor(factory) {
        super("roster", factory);
    }

    render(dataJson, container) {
        var factory = this.factory;
        FormElement.prototype.render.call(this, dataJson, container);

        //QSF text
        dataJson["qsfText-lang"] = factory.resolveLabelLanguage(dataJson.qsfText);
        dataJson["label-lang"] = factory.resolveLabelLanguage(dataJson.label);

        //item language labels
        if (dataJson.horizontal && dataJson.horizontal.items && dataJson.horizontal.items.length > 0) {
            dataJson.horizontal.items.forEach(function (item) {
                item["qsfText-lang"] = factory.resolveLabelLanguage(item.qsfText);
                item["dcfLabel-lang"] = factory.resolveLabelLanguage(item.dcfLabel);
            });
        }

        //Values
        if (dataJson.horizontal.occurrences && dataJson.horizontal.occurrences.length > 0) {
            dataJson.horizontal.occurrences.forEach(function (occ) {
                if (occ.cells && occ.cells.length > 0) {
                    occ.cells.forEach(function (cell) {
                        //rendering textbox
                        if (cell.showTextBox) {
                            cell["value"] = factory.renderValue(cell.value);
                        }

                        if (cell.valueSets && cell.valueSets.length > 0) {
                            cell.valueSets.forEach(function (vs) {
                                if (vs.values.length > 0) {
                                    vs.values.forEach(function (v) {
                                        v["label-lang"] = factory.resolveLabelLanguage(v.label);
                                        if (v.value) {
                                            v["value"] = factory.renderValue(v.value);
                                        }
                                    });
                                }
                            });
                        }
                    });
                }
            });
        }

        if (!dataJson["flat"]) {
            //rendering both horizontal and vertical rosters
            //then hiding one per roster orientation property
            var hTemplateName = factory.getTemplateName(`${this.type}HorizontalTemplate`);
            var vTemplateName = factory.getTemplateName(`${this.type}VerticalTemplate`);
            var hc = $('<div class="horizontal-roster-container"></div>')
                .append(factory.templates[hTemplateName](dataJson));
            if (dataJson.orientation !== "Horizontal")
                hc.css("display", "none");
            $(container).append(hc);

            var vc = $('<div class="vertical-roster-container"></div>')
                .append(factory.templates[vTemplateName](dataJson));
            if (dataJson.orientation !== "Vertical")
                vc.css("display", "none");
            $(container)
                .append(vc);
        } else {
            var fTemplateName = factory.getTemplateName(`${this.type}FlatTemplate`);
            var fc = $('<div class="flat-roster-container"></div>')
                .append(factory.templates[fTemplateName](dataJson));
            $(container).append(fc);

            //appending children
            if (dataJson.flat.occurrences) {
                for (var i = 0; i < dataJson.flat.occurrences.length; i++) {
                    var frItemsContainer = $(`#flatRosterItemsContainer${dataJson.id}_${i + 1}`);
                    dataJson.flat.occurrences[i].cells.forEach(function (elementJson) {
                        factory
                            .formElementTypes[elementJson.type]
                            .render(elementJson, frItemsContainer[0]);
                    });
                }
            }
        }
    }

    preproc(dataJson) {
        FormElement.prototype.preproc.call(this, dataJson);

        if (this.factory.flatRosters == true) {
            dataJson["flat"] = true;
        }

        if (!dataJson["flat"]) {
            //flipping rosters data        
            this.flipRoster(dataJson);
        } else {
            this.flattenRoster(dataJson);
        }
    }

    postProc() {
        FormElement.prototype.postProc.call(this);

        //nothing to do here
    }

    flipRoster(roster) {
        var cells = [];
        var occ = [];
        if (roster.horizontal.occurrences) {
            for (var i = 0; i < roster.horizontal.occurrences.length; i++) {
                occ.push(
                    {
                        "index": roster.horizontal.occurrences[i].index
                    }
                );
                for (var j = 0; j < roster.horizontal.occurrences[i].cells.length; j++) {
                    if (cells.length <= j)
                        cells.push(
                            {
                                "item": roster.horizontal.items[j],
                                "index": (j + 1),
                                "occurrences": []
                            }
                        );

                    cells[j].occurrences.push(roster.horizontal.occurrences[i].cells[j]);
                }
            }
        }

        roster["vertical"] = {};
        roster.vertical["cells"] = cells;
        roster.vertical["occurrences"] = occ;
    }

    flattenRoster(roster) {
        roster.flat = {};
        if (roster.horizontal.occurrences) {
            //roster.flat = structuredClone(roster.horizontal);
            roster.flat = roster.horizontal;

            for (var i = 0; i < roster.flat.occurrences.length; i++) {
                //hiding all occurrences except first one
                roster.flat.occurrences[i]["show"] = (i == 0);

                var newCells = [];
                var block = null;
                for (var j = 0; j < roster.flat.occurrences[i].cells.length; j++) {
                    //memoizing capture type, as it comes from the cell, not the item
                    let ct = roster.flat.occurrences[i].cells[j].captureType;

                    var item = roster.flat.items[j];
                    var itemProps = Object.keys(item);
                    if (itemProps) {
                        for (var k = 0; k < itemProps.length; k++) {
                            roster.flat.occurrences[i].cells[j][itemProps[k]] = item[itemProps[k]];
                        }
                    }

                    //reassigning capture type
                    if (ct) {
                        roster.flat.occurrences[i].cells[j].captureType = ct;
                    }

                    //handling blocks
                    if (block == null && roster.flat.occurrences[i].cells[j].blockStart) {
                        block = {
                            type: "block",
                            name: "",
                            label: roster.flat.occurrences[i].cells[j].block.label,
                            qsfText: roster.flat.occurrences[i].cells[j].block.qsfText,
                            items: []
                        };
                        newCells.push(block);
                    }

                    if (block != null) {
                        //pushing item inside the block
                        block.items.push(roster.flat.occurrences[i].cells[j]);
                    } else {
                        newCells.push(roster.flat.occurrences[i].cells[j]);
                    }

                    if (block != null && roster.flat.occurrences[i].cells[j].blockEnd) {
                        block = null;
                    }
                }
                roster.flat.occurrences[i].cells = newCells;
            }
        }
    }
}

class CaseViewFactory {
    container = {};
    templates = {};
    currentLanguageIdx = 0;
    _currentLanguageId = "";
    get currentLanguageId() {
        return this._currentLanguageId;
    }
    set currentLanguageId(langId) {
        if (langId == this._currentLanguageId)
            return;

        this._currentLanguageId = langId;
        this.renderPage(this.dataJson);
    }
    showLanguageBar = true;

    dataJson = {};
    renderFrame = true;
    showSearchBox = false;
    noteDialog = {};
    imageDialog = {};

    doPrintQuestionnaire = false;
    flatRosters = false;
    maxRosterOcc = 0;
    printAllValueSets = false;

    formElementTypes = {
        "item": new Item(this),
        "block": new Block(this),
        "roster": new Roster(this)
    };

    elementId = 0;

    constructor() {

    }

    readTextFile(file, callback) {
        var rawFile = new XMLHttpRequest();
        rawFile.overrideMimeType("application/json");
        rawFile.open("GET", file, true);
        rawFile.onreadystatechange = function () {
            if (rawFile.readyState === 4 && rawFile.status == "200") {
                //console.log(rawFile.responseText);
                callback(rawFile.responseText);
            }
        }
        rawFile.send(null);
    }

    readJsonFile(file, callback) {
        fetch(file)
            .then(response => response.json())
            .then(json => callback(json));
    }

    resolveLabelLanguage(labels) {
        if (!Array.isArray(labels)) {
            if (labels && labels[this._currentLanguageId])
                return labels[this._currentLanguageId]

            if (labels && labels.length && labels.length > 0)
                return labels;

            return "";
        }

        var idx = this.currentLanguageIdx;
        if (labels.length <= idx)
            idx = 0;

        return labels[idx];
    }

    addStyle(container, style) {

    }

    showSpinner() {
        this.hideSpinner();
        $('<div id="waitAnim" class="spinner-border" role="status"><span class="visually-hidden">Loading...</span></div>')
            .appendTo($('body'));
    }

    hideSpinner() {
        $('#waitAnim').remove();
    }

    preproc(dataJson) {

        //adding class to body
        this.container.addClass("main-container");
        if (this.doPrintQuestionnaire)
            this.container.addClass("print");

        dataJson["label-lang"] = this.resolveLabelLanguage(dataJson.label);

        //preprocessing element data
        var _this = this
        if (dataJson.forms && dataJson.forms.length > 0) {
            dataJson.forms.forEach(function (form) {
                if (form.elements && form.elements.length > 0) {
                    form.elements.forEach(function (elem) {
                        _this.formElementTypes[elem.type].preproc(elem);
                    });
                }
            });
        }

        this.setScrollSpy();

        //adding classes
        if (dataJson.styles) {
            dataJson.styles.forEach(s => {
                $(`<style>.${s.className}{${s.css}}</style>`).appendTo($('head'));
            });
        }

        //combining language IDs with labels
        dataJson.langIdsLabels = [];
        if (dataJson.languages) {
            for (let i = 0; i < dataJson.languages.length; i++) {
                dataJson.langIdsLabels.push(`${dataJson.langIds[i]}: ${dataJson.languages[i]}`);
            }
        }

        //setting language index
        this.setLanguage(dataJson);

        if (this.showLanguageBar === false) {
            this.dataJson.multLang = false;
        }
    }

    postProc(dataJson) {
        if (!this.doPrintQuestionnaire) {


            this.collapseAll();

            //setting bootstrap tooltips
            $('[data-toggle="tooltip"]').tooltip();

            //alternating row colors in rosters
            $("table.cs-roster tbody tr:odd").css("background-color", "rgba(0,0,100,.05)");


            this.setNas();


            this.setRosterFliptButtonClick();

            //rendering all barcodes on a page
            JsBarcode(".barcode").init();

            //building side menu
            this.buildSideMenu($('#sideMenuList'));

            //profiling language dropdown
            this.setLanguageDropdown(dataJson);

            //search button click
            $(".btn-search").click(function () {
                setTimeout(function () {
                    $(".input-search").focus();
                }, 500);
            });

            this.setNoteModal();
            this.setImageModal();

            if (!this.showSearchBox)
                $('.search-box').hide();

            this.setSmoothScrolling();
        }

        //configuring blocks
        this.configureBlocks();
    }

    collapseAll() {
        var _this = this;
        $(".vs-collapse-button").each(function () {
            if (_this.dataJson.noCase === true) {
                $(this).removeClass("collapsed");
            }
            var vs = $(this).closest(".collapse-parent").find(".value-set");
            vs.children(".value-row").each(function () {
                if (_this.dataJson.noCase === true || $(this).find(".checked").length > 0) {
                    $(this).show();
                }
            });
        });

        $(".vs-collapse-button").click(function () {
            _this.toggleVsCollapse(this);
        });
    }

    toggleVsCollapse(elem) {
        $(elem).toggleClass("collapsed");

        var vs = $(elem).closest(".collapse-parent").find(".value-set");

        if ($(elem).hasClass("collapsed")) {
            vs.children(".value-row").each(function () {
                if ($(this).find(".checked").length === 0)
                    $(this).slideUp("fast");
            });
            vs.find(".hidden-range").css("display", "none");
        } else {
            vs.children(".value-row").slideDown("fast");
            vs.find(".hidden-range").css("display", "table-cell");
        }
    }

    setNas() {
        var noCase = this.dataJson["noCase"];
        $(".value-set").each(function () {
            if ($(this).find(".checked").length == 0) {
                if (noCase === false) {
                    $(this).closest(".cell-parent").addClass("not-app");
                }
            }
        });
    }

    setRosterFliptButtonClick() {
        var _this = this;
        $(".roster-transpose-button.horizontal").click(function () {
            var rc = $(this).closest(".horizontal-roster-container");

            rc.hide(200);
            rc.next().show(200, () => _this.showHideMenuItems($('#sideMenuList')));
        });
        $(".roster-transpose-button.vertical").click(function () {
            var rc = $(this).closest(".vertical-roster-container");
            rc.hide(200);
            rc.prev().show(200, () => _this.showHideMenuItems($('#sideMenuList')));
        });
    }

    buildSideMenu(menuContainer) {
        function getIndentLevel(item) {
            var res = 0;
            item.parents().each(function () {
                if ($(this).hasClass("menu-item-container"))
                    res++;
            });

            return res;
        }

        $('div.side-menu-anchor').remove();

        menuContainer.empty();
        var indent = 10; //number of pixels to indent per level
        var i = 0;
        $(".menu-item").each(function () {
            //adding anchor tags to each menu-item
            $(`<div id="i${i}" class="side-menu-anchor" style="position: relative;top: -50px;"></div>`)
                .prependTo($(this));

            //indent level
            var il = getIndentLevel($(this));

            //adding a class for container items
            var pClass = $(this).hasClass("menu-item-container") ?
                " menu-item-container-div" :
                "";

            //adding menu item
            $(`<li class="nav-item menu-entry" aid="i${i}"></li>`)
                .appendTo(menuContainer)
                .html(`<div class="menu-item-div${pClass}" style="padding-left:${il * indent}px;"><div style="position:absolute;color:#777;font-size:26px;">•</div><a class="nav-link menu-item-link" href="#i${i++}">${$(this).attr("menutext")}</a></div>`);
        });

        this.showHideMenuItems(menuContainer)
    }

    showHideMenuItems(menuContainer) {
        //The side bar includes menu item entries that references to the ids of the items in the view. When the menu entry is clicked 
        //it scrolls to the element in the view. However, when there are rosters, the sidebar entries include both the horizontal and
        //vertical roster elements. This function hides the entries that for either horizontal or vertical rosters depending on which 
        //container is hidden. Using isVisible takes more than 30 seconds on some devices.  Refactored the code to check if the id element
        //the menu entry is refering to is a table cell or header that is contained in a roster and hiding those menu entries on the sidebar
        //as needed. For a given roster, there are two containers in the view depeneding on the orientation the display for the other is turned off
        $(".menu-entry").each(function () {
            let anchorId = $(this).attr("aid"); // Get the ID of the anchor
            let anchorElement = document.getElementById(anchorId); // Get the anchor element
          
            $(this).show(); // Show the menu entry

            // Check if the anchor element exists and is contained in a visible parent table
            if ($(anchorElement).closest('td, th').length > 0) {
                 // Find the closest container with class "vertical-roster-container"
                const rosterContainer = $(anchorElement).closest('.vertical-roster-container, .horizontal-roster-container')[0];
                if(rosterContainer && getComputedStyle(rosterContainer).display == "none") {
                    $(this).hide(); 
                }
            }
        });
    }

    setLanguage(dataJson) {
        let langIdx = dataJson.langIds.indexOf(this._currentLanguageId);

        if (langIdx < 0)
            langIdx = 0;

        this.currentLanguageIdx = langIdx;
        this._currentLanguageId = dataJson.langIds[langIdx];
    }

    setLanguageDropdown(dataJson) {
        $("#langSelect").val(dataJson.langIdsLabels[this.currentLanguageIdx]);

        //setting on value change event
        var _this = this;
        $("#langSelect").change(function () {
            _this.currentLanguageIdx = this.selectedIndex;

            _this._currentLanguageId = dataJson.langIds[this.selectedIndex];

            _this.renderCase(_this.dataJson);
        });
    }

    setNoteModal() {
        //appending modal to body
        this.container.append(this.noteDialog);


        //notes dialog click
        var _this = this
        $(".cs-note").click(function () {
            _this.showNoteModal(
                $(this).children('.cs-note-title').html(),
                $(this).children('.cs-note-text').html()
            );
        });
    }

    setImageModal() {
        //appending modal to body
        this.container.append(this.imageDialog);


        //notes dialog click
        var _this = this
        $("img.cs-thumb").click(function () {
            _this.showImageModal(
                $(this).attr("dialog-title"),
                $(this).attr("src")
            );
        });
    }

    drawRosterBlockBorders(rosterTable) {
        var inBlock = false;
        $(rosterTable).find('tr.cs-roster-row').each(function () {
            if (!inBlock && $(this).hasClass('roster-block-start'))
                inBlock = true;

            if (inBlock) {
                //$(this).prepend('<td class="roster-block"></td>');
                //$(this).append('<td class="roster-block"></td>');

                if ($(this).hasClass('roster-block-start-gap')) {
                    //$(this).children().first().css({ 'border': '1px solid #777', 'border-bottom': 'none', 'border-top': 'none' });
                    $(this).css({ 'display': 'none' });
                } else if ($(this).hasClass('roster-block-end')) {
                    $(this).children().first().css({ 'border-top': '1px solid #777', 'box-shadow': 'inset 0 7px 5px -5px rgb(0 0 0 / 30%)' });
                } else if (!$(this).hasClass('roster-block-start')) {
                    $(this).children().first().css({ 'border-left': '1px solid #777' });
                    $(this).children().last().css({ 'border-right': '1px solid #777' });
                }
            } else {

            }

            if (inBlock && $(this).hasClass('roster-block-end'))
                inBlock = false;
        });

    }

    addBlockHeaders(rosterTable) {
        var blockHeaderRow = null;
        if ($(rosterTable).find('.hroster-block-start-marker')) {
            blockHeaderRow = $("<tr></tr>")
            $(rosterTable).find('thead').prepend(blockHeaderRow);
        }

        var inBlock = false;
        var colSpan = 0;
        $(rosterTable).find('thead tr.col-headers').children().each(function (idx) {
            if (!inBlock && $(this).children().first().hasClass('hroster-block-start-marker')) {
                //adding a cell to the block header row
                $('<th class="hroster-block-header"></th>')
                    .appendTo(blockHeaderRow)
                    .append($(this).children('.hroster-block-start-marker').children());

                inBlock = true;
            }

            if (inBlock) {
                colSpan++;
            } else {
                colSpan = 0;
                $(this)
                    .attr('rowspan', 2)
                    .appendTo(blockHeaderRow);
            }

            if (inBlock && $(this).children().first().hasClass('hroster-block-end-marker')) {
                //blockHeaderTh.attr('rowspan', rowSpan);
                blockHeaderRow.children().last().attr('colspan', colSpan);

                inBlock = false;
            }
        });
    }

    configureBlocks() {
        var _this = this;
        //drawing borders around blocks in vertical rosters
        $('.cs-roster.vertical').each(function () {
            _this.drawRosterBlockBorders(this)
        });

        //adding column headers for blocks in vertical rosters
        $('.cs-roster.horizontal').each(function () {
            _this.addBlockHeaders(this)
        });
    }


    setScrollSpy() {
        $('body').scrollspy({ target: ".nav" })
    }

    showNoteModal(title, text) {
        $('#noteModalLabel').html(title);
        $('#noteModal .modal-body').html(text);
        $('#noteModal').modal();
    }

    showImageModal(title, imageUrl) {
        $('#imageModalLabel').html(title);
        $(`<img src="${imageUrl}" class="cs-image" />`)
            .appendTo($('#imageModal .modal-body').empty());

        //$('#imageModal .modal-body').html(imageUrl);
        $('#imageModal').modal();
    }

    setSmoothScrolling() {
        // Add smooth scrolling to all links in navbar + footer link
        $("#sideMenuList a").on('click', function (event) {

            // Prevent default anchor click behavior
            event.preventDefault();

            // Store hash
            var hash = this.hash;

            // Using jQuery's animate() method to add smooth page scroll
            // The optional number (900) specifies the number of milliseconds it takes to scroll to the specified area
            $('html, body').animate({
                scrollTop: $(hash).offset().top
            }, 500, function () {

                // Add hash (#) to URL when done scrolling (default click behavior)
                window.location.hash = hash;
            });
        });
    }

    getTemplateName(tName) {
        if (this.doPrintQuestionnaire)
            return tName + "Print";

        return tName;
    }

    compileTemplates(templates) {
        this.templates["pageTemplate"] = Handlebars.compile($(templates).filter('#caseViewFrameTemplate').html());
        this.templates["formTemplate"] = Handlebars.compile($(templates).filter('#formTemplate').html());
        this.templates["itemTemplate"] = Handlebars.compile($(templates).filter('#itemTemplate').html());
        this.templates["blockTemplate"] = Handlebars.compile($(templates).filter('#blockTemplate').html());
        this.templates["rosterHorizontalTemplate"] = Handlebars.compile($(templates).filter('#rosterHorizontalTemplate').html());
        this.templates["rosterVerticalTemplate"] = Handlebars.compile($(templates).filter('#rosterVerticalTemplate').html());
        this.templates["rosterFlatTemplate"] = Handlebars.compile($(templates).filter('#rosterFlatTemplate').html());

        this.templates["boxValueTemplate"] = Handlebars.compile($(templates).filter('#boxValueTemplate').html());
        this.templates["barcodeTemplate"] = Handlebars.compile($(templates).filter('#barcodeTemplate').html());

        //questionnaire print templates
        this.templates["pageTemplatePrint"] = Handlebars.compile($(templates).filter('#caseViewFrameTemplatePrint').html());
        this.templates["formTemplatePrint"] = Handlebars.compile($(templates).filter('#formTemplatePrint').html());
        this.templates["itemTemplatePrint"] = Handlebars.compile($(templates).filter('#itemTemplatePrint').html());
        this.templates["blockTemplatePrint"] = Handlebars.compile($(templates).filter('#blockTemplatePrint').html());
        this.templates["rosterHorizontalTemplatePrint"] = Handlebars.compile($(templates).filter('#rosterHorizontalTemplatePrint').html());
        this.templates["rosterVerticalTemplatePrint"] = Handlebars.compile($(templates).filter('#rosterVerticalTemplatePrint').html());
        this.templates["rosterFlatTemplatePrint"] = Handlebars.compile($(templates).filter('#rosterFlatTemplatePrint').html());

        this.templates["boxValueTemplatePrint"] = Handlebars.compile($(templates).filter('#boxValueTemplatePrint').html());
        this.templates["barcodeTemplatePrint"] = Handlebars.compile($(templates).filter('#barcodeTemplatePrint').html());
    }

    renderTestCase(testInputJsonFile) {
        this.readJsonFile(testInputJsonFile, function (json) {
            this.doPrintQuestionnaire = false;
            this.renderCase(json);
        }.bind(this));
    }

    printTestQuestionnaire(testInputJsonFile) {
        this.readJsonFile(testInputJsonFile, function (json) {
            this.doPrintQuestionnaire = true;
            this.renderCase(json);
        }.bind(this));
    }

    renderCase(dataJson) {
        this.dataJson = dataJson;
        //compiling templates if not already done
        if (Object.keys(this.templates).length === 0) {
            $.get('/questionnaire-view/Templates.html', function (templates, textStatus, jqXhr) {
                this.compileTemplates(templates);

                this.noteDialog = $(templates).filter('#noteModal');
                this.imageDialog = $(templates).filter('#imageModal');

                this.renderPage(dataJson);
            }.bind(this));
            return;
        }

        this.renderPage(dataJson);
    }

    renderPage(dataJson) {
        if (!("forms" in dataJson))
            return;
            
        this.showSpinner();
        setTimeout(() => {
            if (this.container === undefined ||
                this.container === null ||
                jQuery.isEmptyObject(this.container)) {
                this.container = $('body');
            }
            this.preproc(dataJson);

            if (this.renderFrame) {
                this.container
                    .empty()
                    .append(this.templates[this.getTemplateName('pageTemplate')](dataJson));
            }
            else {
                this.container
                    .empty()
                    .append($('<div class="container page-container"></div>'));
            }

            //rendering forms
            dataJson.forms.forEach(function (formJson) {
                this.renderForm(formJson, this.elementId++);
            }.bind(this));

            this.postProc(dataJson);

            this.hideSpinner();
        }, 100);
    }

    renderForm(formDataJson, id) {
        formDataJson["id"] = id;
        //adding current language label
        formDataJson["label-lang"] = this.resolveLabelLanguage(formDataJson.label);
        $('.page-container').append(this.templates[this.getTemplateName('formTemplate')](formDataJson));

        //rendering items
        if (!formDataJson.elements)
            return;

        formDataJson.elements.forEach(function (elementJson) {
            this
                .formElementTypes[elementJson.type]
                .render(elementJson, $('#element-container' + formDataJson.id)[0]);
        }.bind(this));
    }

    renderValue(valueJson) {
        if (valueJson.type) {
            if (valueJson.type === "textbox")
                return this.renderTextbox(valueJson.value,
                    valueJson.from,
                    valueJson.to,
                    valueJson.size,
                    valueJson.alignRight,
                    valueJson.floatRight
                );
            else if (valueJson.type === "barcode")
                return this.renderBarcode(valueJson.value);
        }

        return valueJson;
    }

    renderTextbox(value, from, to, size, alignRight, floatRight) {
        var vInput = {
            "cells": [],
            "from": from,
            "to": to.trim().length > 0 ? ":" + to : to
        };

        if (floatRight === true)
            vInput["marginRight"] = 0
        else
            vInput["marginRight"] = "auto";

        var valStr = value.toString();

        if (valStr.length == 0) {
            vInput["naClass"] = "not-app-box";
        }

        var startIdx = alignRight ? size - valStr.length : 0;
        for (var i = 0; i < size; i++)
            vInput.cells.push({
                "digit": function (index, startIdx, valStr) {
                    var valIdx = i - startIdx;

                    if (valIdx >= 0 && valIdx < valStr.length)
                        return valStr.substring(valIdx, valIdx + 1);

                    return "";
                }(i, startIdx, valStr),
                "showTick": i > 0
            });

        return this.templates[this.getTemplateName('boxValueTemplate')](vInput);
    }

    renderBarcode(value) {
        var bInput = {
            "value": value,
        };

        return this.templates[this.getTemplateName('barcodeTemplate')](bInput);
    }

    ///Convert JSON data coming from CSPro to a format compatible with this factory
    convertJson(inputJson) {
        let ip = new CaseViewInputProcessor(this);
        ip.convertInput(inputJson);

        return ip.result;
    }
}

