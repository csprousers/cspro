const tagName = 'cspro-chart';
import '/external/chartjs/chart.umd.js';

class CSProChart extends HTMLElement {
    constructor() {
        super();

        this.chartType = null;
        this.name = null;
        this.label = null;
        this.chartData = null;
        this.chartOptions = null;
    }

    getContent() {
        return `
            <div class="chart-area">
                <div class="chart-content-area">
                    <canvas></canvas>
                </div>
            </div>
        `;
    }

    constructPayload() {
        let payload = {};
        payload['chartData'] = [];
        payload['chartOptions'] = [];

        if (this.attributes.getNamedItem('name')) {
            const name = this.attributes.getNamedItem('name').value;
            if (name && name.length > 0) {
                payload['name'] = name;
                this.name = name;
            }
        }

        if (this.attributes.getNamedItem('label')) {
            const label = this.attributes.getNamedItem('label').value;
            if (label && label.length > 0) {
                payload['label'] = label;
                this.label = label;
            }
        }

        if (this.attributes.getNamedItem('chartType')) {
            const chartType = this.attributes.getNamedItem('chartType').value;
            if (chartType && chartType.length > 0) {
                payload['chartType'] = chartType;
                this.chartType = chartType;
            }
        }

        if (this.attributes.getNamedItem('chartData')) {
            const chartData = this.attributes.getNamedItem('chartData').value;
            payload['chartData'] = JSON.parse(chartData);
            this.chartData = chartData;
        }

        if (this.attributes.getNamedItem('chartOptions')) {
            const chartOptions = this.attributes.getNamedItem('chartOptions').value;
            payload['chartOptions'] = JSON.parse(chartOptions);
            this.chartOptions = chartOptions;
        }

        return payload;
    }

    getInboundData(payload) {
        const constructedPayload = {};
        if (payload['chartType']) {
            constructedPayload['type'] = payload['chartType'];
        }

        if (payload['chartData']) {
            constructedPayload['data'] = payload['chartData'];
        }

        if (payload['chartOptions']) {
            constructedPayload['options'] = payload['chartOptions'];
        }

        return constructedPayload;
    }

    connectedCallback() {
        const payload = this.constructPayload();
        const content = this.getContent(payload);

        const divContainer = document.createElement('div');
        divContainer.innerHTML = content;

        if (!this.shadowRoot) {
            this.attachShadow({ mode: 'open' });
            this.shadowRoot.appendChild(divContainer.cloneNode(true));
        }
    }

    drawChart(chartData = null, chartOptions = null) {
        const payload = this.constructPayload();
        if (chartData !== null) {
            payload['chartData'] = chartData;
        }
        if (chartOptions !== null) {
            payload['chartOptions'] = chartOptions;
        }
        const canvas = this.shadowRoot.querySelector("canvas");

        const inboundData = this.getInboundData(payload);

        if (canvas) {
            new Chart(canvas, inboundData);
        }
    }
}

const register = () => customElements.define(tagName, CSProChart);
window.WebComponents ? window.WebComponents.waitFor(register) : register();
