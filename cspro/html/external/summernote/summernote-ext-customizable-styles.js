/**
 * Summernote plugin that adds a style menu that lets users define their own styles.
 *
 *
 */

(function (factory) {
    if (typeof define === 'function' && define.amd) {
        define(['jquery'], factory);
    } else if (typeof module === 'object' && module.exports) {
        module.exports = factory(require('jquery'));
    } else {
        factory(window.jQuery);
    }
}(function ($) {

    const CustomizableStylePlugin = function (context) {

        const ui = $.summernote.ui;
        const range = $.summernote.range;
        const dom = $.summernote.dom;
        const lists = $.summernote.lists;
        const options = context.options;
        const lang = options.langInfo;
        const styleConverter = context.modules.editor.style;
        const editor = context.modules.editor;

        this.shouldInitialize = () => options.customizableStyle.enable !== false;

        const addStyleToDom = (className, style) => {
            // Adds the css class for the style to the current document
            const selectorText = '.' + className;

            let $styleElement = $('#customizable-styles-custom-styles-element');
            if ($styleElement.length === 0)
                $styleElement = $('<style id="customizable-styles-custom-styles-element" />').appendTo('body');
            const sheet = $styleElement[0].sheet;
            let existingRuleIndex = -1;
            for (let i = 0; i < sheet.cssRules.length; ++i) {
                if (sheet.cssRules[i].selectorText === selectorText) {
                    existingRuleIndex = i;
                }
            }
            if (existingRuleIndex >= 0)
                sheet.deleteRule(existingRuleIndex);

            const rule = selectorText + ' {' + style + '}';
            sheet.insertRule(rule);
        };

        const parseCSS = (css) => {
            const doc = document.implementation.createHTMLDocument("");
            const styleElement = document.createElement("style");
            styleElement.textContent = "p {" + css + "}";
            doc.body.appendChild(styleElement);
            const style = styleElement.sheet.cssRules[0].style;
            const result = new Map();
            for (var i = 0; i < style.length; ++i) {
                const name = style[i];
                const val = style.getPropertyValue(name);
                result.set(name, val);
            }
            return result;
        }

        const setDefaultStyle = (style) => {
            // Sets the default text style for the editor
            const editable = context.layoutInfo.editable;
            parseCSS(style).forEach((value, key) => { editable.css(key, value)});
        };

        const makeValidClassName = (name) => {
            if (name[0].match(/[^a-zA-Z]/g))
                name = 'Z' + name;
            return name.replace(/[^a-zA-Z0-9_-]/g, "-");
        };

        // Get set of first block level ancestor of all nodes in a list
        // e.g. if you have the following HTML <p>foo <span>bar</span></p><h1>baz</h1>
        // and nodes = ['<span>bar</span>', '<h1>baz</h1>']
        // the result will be the <p> and <h1> nodes
        const getBlockAncestorNodes = (nodes) => {
            return lists.compact(lists.unique(nodes.map((n) => dom.ancestor(n, dom.isPara))));
        };

        // Change all block level nodes in a range to use the new tag nodeName
        // Splits nodes as needed to only apply the new block to the selected
        // part of partially selected nodes.
        const applyBlockTag = (rng, nodeName) => {
            rng = rng.splitText();
            if (rng.isCollapsed()) {
                return [rng.insertNode(dom.create(nodeName))];
            }

            let nodes = rng.nodes();
            let blockParentNodes = getBlockAncestorNodes(nodes);
            if (blockParentNodes || blockParentNodes.length === 0) {
                // No top level block node anywhere in the editor text - add one in
                // so that below code will work
                context.layoutInfo.editable.contents().wrapAll('<p>');
                blockParentNodes = getBlockAncestorNodes(nodes);
            }

            let startPoint = rng.getStartPoint();
            let endPoint = rng.getEndPoint();
            const newParents = blockParentNodes.map((parent) => {
                if (parent.contains(startPoint.node) && !dom.isLeftEdgePointOf(startPoint, parent)) {
                    parent = dom.splitTree(parent, startPoint);
                }
                if (parent.contains(endPoint.node) && !dom.isRightEdgePointOf(endPoint, parent)) {
                    dom.splitTree(parent, endPoint);
                }
                return dom.replace(parent, nodeName);
            });

            nodes = nodes.filter((n) => !lists.contains(blockParentNodes, n)).concat(newParents);

            return nodes;
        };

        this.applyStyle = (styleName) => {
            editor.beforeCommand.apply(editor);
            applyStyleToDom(styleName);
            editor.afterCommand.apply(editor);
        };

        const applyStyleToDom = (title) => {
            const style = options.styleTags.find(t => t.title == title);
            const tag = style.tag;
            const className = style.className;
            let rng = editor.getLastRange();
            const spans = dom.isPara(dom.create(tag)) ?
                applyBlockTag(rng, tag) :
                editor.style.styleNodes(rng, {nodeName: tag});
            if ($(spans).className)
                $(spans).removeClass();
            if (className)
                $(spans).addClass(className);

            // This is copied from fontStyling() in Editor.js
            // It handles the case where there is no selection
            // and you need to insert a new span with the new format
            // so that when you start typing it inserts with the new style
            if (rng.isCollapsed()) {
                const firstSpan = lists.head(spans);
                if (firstSpan && !dom.nodeLength(firstSpan)) {
                    firstSpan.innerHTML = dom.ZERO_WIDTH_NBSP_CHAR;
                    range.createFromNode(firstSpan.firstChild).select();
                    editor.setLastRange();
                    editor.$editable.data(editor.KEY_BOGUS, firstSpan);
                }
            } else {
                editor.setLastRange(
                    editor.createRangeFromList(spans).select(),
                );
            }
        };

        this.addStyle = (name, style) => {
            const className = makeValidClassName(name);
            addStyleToDom(className, style);
            context.triggerEvent('customizableStyle.styleAdded', name, style);
        };

        this.getStyles = () => {
            return options.styleTags;
        };

        this.setStyles = (styleTags) => {
            options.styleTags = styleTags;
            styleTags.forEach((t) => {
                if (t.className && t.style) {
                    addStyleToDom(t.className, t.style);
                }
            });
            setDefaultStyle(styleTags[0].style);
            context.triggerEvent('customizableStyle.stylesUpdated', styleTags);
        };

        this.currentStyle = () => {
            const currentStyle = context.invoke('editor.currentStyle');
            currentStyle['color'] = document.queryCommandValue('foreColor');
            currentStyle['background-color'] = document.queryCommandValue('backColor');
            if (currentStyle.ancestors) {
                for (a of currentStyle.ancestors) {
                    if (a.className) {
                        currentStyle.class = a.className;
                        break;
                    }
                }
            }
            if (!currentStyle.class)
                currentStyle.class = options.styleTags[0].className;

            return currentStyle;
        };

    };

    $.extend(true, $.summernote, {
        plugins: {
            customizableStyle: CustomizableStylePlugin,
        },

        options: {
            customizableStyle: {
                enable: true,
            },
        },
    });
}));
