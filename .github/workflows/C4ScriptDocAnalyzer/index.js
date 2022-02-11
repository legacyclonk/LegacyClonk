const fs = require('fs');

//The following array will contain all C4Script functions defined in C4Script.cpp
const engineFunctions = filterFile('./src/C4Script.cpp', /AddFunc\(pEngine, +"(\w+)", +Fn\w+(, +(false|true))?\);/gm, 1)
console.log(`Loaded C4Script.cpp`);

//The following array will contian all C4Script functions documented in the lcdocs
const lcdocsFunctions = filterFile('.github/workflows/C4ScriptDocAnalyzer/lcdocs_functions.txt', /^Name=(\w*)/gm, 1);
console.log(`Loaded generated file with C4Script functions documented in the current lcdocs master https://github.com/legacyclonk/lcdocs`);

console.log(`The following C4Script functions are defined in the engine but not documented in the current lcdocs master:\n`);
let undocumentedFunctions = 0;
engineFunctions.forEach(c4ScriptFunctionName => {
    if (!lcdocsFunctions.includes(c4ScriptFunctionName)) {
        console.log(`\t${c4ScriptFunctionName}()`);
        undocumentedFunctions++;
        process.exitCode = 1;
    }
});

console.log(`\nThe following C4Script functions are documented in the current lcdocs master but not defined in the engine:\n`);
let undefinedFunctions = 0;
lcdocsFunctions.forEach(c4ScriptFunctionName => {
    if (!engineFunctions.includes(c4ScriptFunctionName)) {
        console.log(`\t${c4ScriptFunctionName}()`);
        undefinedFunctions++;
    }
});

console.log(`\nThere are ${engineFunctions.length} defined functions in C4Script.cpp and ${lcdocsFunctions.length} documented functions in lcdocs.`);
console.log(`Exiting with ${undocumentedFunctions} undocumented functions and ${undefinedFunctions} undefined functions.`);



function filterFile(filepath, regex, captureGroup = 1) {
    const file = fs.readFileSync(filepath).toString()
    let functionMatcher = regex.exec(file);
    const functions = [];

    do {
        functions.push(functionMatcher[captureGroup]);
    } while ((functionMatcher = regex.exec(file)) !== null);

    return functions;
}