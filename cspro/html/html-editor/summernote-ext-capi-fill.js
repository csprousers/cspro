(function (factory) {
    if (typeof define === 'function' && define.amd) {
        // AMD. Register as an anonymous module.
        define(['jquery'], factory);
    } else if (typeof module === 'object' && module.exports) {
        // Node/CommonJS
        module.exports = factory(require('jquery'));
    } else {
        // Browser globals
        factory(window.jQuery);
    }
}(function($) {

    const CapiFillPlugin = function(context) {

        const range = $.summernote.range;
        const $editable = context.layoutInfo.editable;
        const $container = context.options.container;

        //right to left direction change - from plugin summernote-ext-rtl.js
        this.rtl = () => {
            function clearSelection() {
                if (document.selection) {
                    document.selection.empty();
                } else if (window.getSelection) {
                    window.getSelection().removeAllRanges();
                }
            }

            function getHTMLOfSelection() {
                var range;
                if (document.selection && document.selection.createRange) {
                    range = document.selection.createRange();
                    return range.htmlText;
                } else if (window.getSelection) {
                    selection = window.getSelection();
                    if (selection.rangeCount > 0) {
                        range = selection.getRangeAt(0);
                        var clonedSelection = range.cloneContents();
                        var div = document.createElement('div');
                        div.appendChild(clonedSelection);
                        return div.innerHTML;
                    } else {
                        return '';
                    }
                } else {
                    return '';
                }
            }
            var highlight = window.getSelection();
            var range = highlight.getRangeAt(0);
            var elementsClass = range.endContainer.parentElement;


            window.highlight = highlight;
            window.range = range;
            window.elementsClass = elementsClass;
            if (elementsClass.style.direction != "rtl" && elementsClass.style.direction != "ltr") {
                var spn = document.createElement('div');
                spn.innerHTML = getHTMLOfSelection();
                spn.style.direction = 'rtl';
                range.deleteContents();
                range.insertNode(spn);
            } else {
                elementsClass.style.direction = 'rtl';
                if ($(elementsClass).is("li")) {
                    direction = $(elementsClass).css('direction');
                    $(elementsClass).parent().css('direction', direction);
                }
            }
            clearSelection();
        };

        //left to right direction change - from plugin summernote-ext-rtl.js
        this.ltr = () => {
            function clearSelection() {
                if (document.selection) {
                    document.selection.empty();
                } else if (window.getSelection) {
                    window.getSelection().removeAllRanges();
                }
            }

            function getHTMLOfSelection() {
                var range;
                if (document.selection && document.selection.createRange) {
                    range = document.selection.createRange();
                    return range.htmlText;
                } else if (window.getSelection) {
                    selection = window.getSelection();
                    if (selection.rangeCount > 0) {
                        range = selection.getRangeAt(0);
                        var clonedSelection = range.cloneContents();
                        var div = document.createElement('div');
                        div.appendChild(clonedSelection);
                        return div.innerHTML;
                    } else {
                        return '';
                    }
                } else {
                    return '';
                }
            }
            var highlight = window.getSelection();
            var range = highlight.getRangeAt(0);
            var elementsClass = range.endContainer.parentElement;
            if (elementsClass.style.direction != "rtl" && elementsClass.style.direction != "ltr") {
                var spn = document.createElement('div');
                spn.innerHTML = getHTMLOfSelection();
                spn.style.direction = 'ltr';
                range.deleteContents();
                range.insertNode(spn);
            } else {
                elementsClass.style.direction = 'ltr';
                if ($(elementsClass).is("li")) {
                    direction = $(elementsClass).css('direction');
                    $(elementsClass).parent().css('direction', direction);
                }
            }
            clearSelection();
        };

        this.events = {
            'summernote.change': () => update(),
            'summernote.scroll': () => updatePositions(),
        };

        $(window).resize(() => updatePositions());

        $container.mousemove(event => {
            const hit = fills.find((fill) => {
                if (!fill.errorMessage)
                    return false;
                const x = event.clientX - fill.bounds.x;
                const y = event.clientY - fill.bounds.y;
                if (x >= 0 && x <= fill.bounds.width && y >= 0 && y <= fill.bounds.height)
                    return true;
                if (!fill.errorDotNode && fill.errorDotNode.length)
                    return false;
                const dotRect = fill.errorDotNode[0].getBoundingClientRect();
                return event.clientX >= dotRect.left && event.clientX <= dotRect.right
                    && event.clientY >= dotRect.top && event.clientY <= dotRect.bottom;
            });

            if (hit)
                showTooltip(event.clientX, event.clientY, hit.errorMessage);
            else
                hideTooltip();
        });

        let fills = [];
        let syntaxErrors = [];

        this.setSyntaxErrors = (errors) => {
            syntaxErrors = errors;
            updateErrorMarkers();
        };

        const clear = () => {
            fills.forEach((fill) => {
                if (fill.highlightNode)
                    fill.highlightNode.remove();
                if (fill.errorDotNode)
                    fill.errorDotNode.remove();
            });
            fills = [];
        };

        const update = () => {
            clear();
            fills = $editable.map((i, n) => getFills(n)).get();
            if (cleanFills(fills))
                fills = $editable.map((i, n) => getFills(n)).get();
            fills.forEach(createFillNode);
            updateErrorMarkers();
        };

        const updatePositions = () => {
            fills.forEach(updatePosition);
        };

        const createFillNode = (fill) => {
            fill.highlightNode =
                $('<div class="cspro-capi-fill" />')
                    .css({
                        left: fill.bounds.x - 2,
                        top: fill.bounds.y - 5,
                        width: fill.bounds.width + 1,
                        height: fill.bounds.height + 10,
                    })
                    .appendTo($container);
        };

        const createErrorDot = (fill) => {
            fill.errorDotNode =
                $('<div class="cspro-capi-fill-error-dot" />')
                    .css({
                        left: 0,
                        top: fill.bounds.y + fill.bounds.height/2 - 4,
                    })
                    .appendTo($container);
        };

        const updatePosition = (fill) => {
            fill.bounds = fill.range.getBoundingClientRect();
            fill.highlightNode.css({
                left: fill.bounds.x - 2,
                top: fill.bounds.y - 5,
                width: fill.bounds.width + 1,
                height: fill.bounds.height + 10,
            });
            if (fill.errorDotNode) {
                fill.errorDotNode.css('top', fill.bounds.y + fill.bounds.height/2 - 4);
            }
        };

        const updateErrorMarkers = () => {
            for (const fill of fills) {
                fill.errorMessage = syntaxErrors[fill.text];
                if (fill.errorMessage) {
                    fill.highlightNode.addClass('cspro-capi-fill-error');
                    if (!fill.errorDotNode)
                        createErrorDot(fill);
                } else {
                    fill.highlightNode.removeClass('cspro-capi-fill-error');
                    if (fill.errorDotNode) {
                        fill.errorDotNode.remove();
                        fill.errorDotNode = null;
                    }
                }
            }
        };

        const showTooltip = (clientX, clientY, message) => {
            let tooltip = $('.cspro-capi-fill-error-tooltip');
            if (!tooltip.length) {
                tooltip = $('<div class="cspro-capi-fill-error-tooltip" />');
                $container.append(tooltip);
            }
            tooltip.text(message);
            tooltip.css({left: clientX + 20, top: clientY - 20});
            tooltip.show();
        };

        const hideTooltip = () => {
            $container.find(".cspro-capi-fill-error-tooltip").hide();
        };

        const delimiterRegex = /~{2,3}/;

        const findDelimitersInDom = (node) => {
            const delimiters = [];

            const treeWalker = document.createTreeWalker(
                node,
                NodeFilter.SHOW_TEXT,
            );

            let currentNode = treeWalker.currentNode;
            let currentDelimiter = null;
            while (currentNode) {
                let nodeText = currentNode.nodeValue;
                let offset = 0;
                while (nodeText) {
                    if (currentDelimiter) {
                        const match = nodeText.match(currentDelimiter);
                        if (match) {
                            delimiters.push({node: currentNode, offset: match.index + offset, delimiter: currentDelimiter});
                            nodeText = nodeText.slice(match.index + currentDelimiter.length);
                            offset += match.index + currentDelimiter.length;
                            currentDelimiter = null;
                        } else {
                            nodeText = null;
                        }
                    } else {
                        const match = nodeText.match(delimiterRegex);
                        if (match) {
                            currentDelimiter = match[0];
                            delimiters.push({node: currentNode, offset: match.index + offset, delimiter: currentDelimiter});
                            nodeText = nodeText.slice(match.index + currentDelimiter.length);
                            offset += match.index + currentDelimiter.length;
                        } else {
                            nodeText = null;
                        }
                    }
                }
                currentNode = treeWalker.nextNode();
            }
            return delimiters;
        };

        const getFillRanges = (node) => {
            const ranges = [];
            const delimiters = findDelimitersInDom(node);
            let iFirst = 0;
            while (delimiters.length - iFirst >= 2) {
                const iSecond = iFirst + 1;
                const delimiterLength = delimiters[iFirst].delimiter.length;
                const inner = range.create(delimiters[iFirst].node,
                    delimiters[iFirst].offset + delimiterLength,
                    delimiters[iSecond].node,
                    delimiters[iSecond].offset).nativeRange();
                const outer = range.create(delimiters[iFirst].node,
                    delimiters[iFirst].offset,
                    delimiters[iSecond].node,
                    delimiters[iSecond].offset + delimiterLength).nativeRange();
				    if(delimiters[iFirst].node == delimiters[iSecond].node){
					    ranges.push({inner: inner, outer: outer});
				    }
                iFirst = iSecond + 1;
            }
            return ranges;
        };

        const getFills = (node) => {
            const ranges = getFillRanges(node);
            return ranges.map((r) => ({range: r.inner, outerRange: r.outer, text: r.inner.toString(), bounds: r.inner.getBoundingClientRect()}));
        };

        const removeFormatting = (range) => {
            const text = range.toString();
            range.deleteContents();
            range.insertNode(document.createTextNode(text));
        };

        const cleanFills = (fills) => {
            let cleaned = false;
            for (const fill of fills.reverse()) {
                if (fill.outerRange.startContainer !== fill.outerRange.endContainer) {
                    removeFormatting(fill.outerRange);
                    cleaned = true;
                }
            }
            return cleaned;
        };
    };

    // Extends summernote
    $.extend(true, $.summernote, {
        plugins: {
            capifill: CapiFillPlugin,
        },
    });
}));
