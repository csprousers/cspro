
function getVariableNameFromCSProFrequency(csproData) {
    return !csproData.variable ? undefined : csproData.variable.name;
}


function getChartJsDataFromCSProFrequency(csproData) {
    const labels = new Array();
    const data = new Array();

    csproData.rows.forEach((csproRow) => {
        // if no label is present, use the values (when present)
        labels.push(csproRow.label  ? csproRow.label :
                    csproRow.values ? csproRow.values :
                                      []);

        data.push(csproRow.count);
    });

    const variableLabel = !csproData.variable      ? undefined :
                          csproData.variable.label ? csproData.variable.label :
                                                     csproData.variable.name;

    return {
        labels: labels,
        datasets: [
            {
                label: variableLabel,
                data: data
            }
        ]
    };
}


function getChartJsOptionsForFrequency(csproData, chartType) {
    let displayLegend;
    let scales;

    if( chartType == "bar" || chartType == "line" ) {
        displayLegend = false;
        scales = {
            y: {
                title: {
                    display: true,
                    text: "Frequency"
                }
            }
        };

        const xLabel = !csproData.variable      ? undefined :
                       csproData.variable.label ? `${csproData.variable.name}: ${csproData.variable.label}` :
                                                  csproData.variable.name;
        if( xLabel ) {
            scales.x = {
                title: {
                    display: true,
                    text: xLabel
                }
            };
        }
    }
    else {
        displayLegend = true;
        scales = false;
    }

    return {
        scales: scales,
        animation: true,
        plugins: {
            legend: {
                display: displayLegend,
                position: "bottom"
            },
            tooltip: {
                enabled: true
            }
        }
    };
}
