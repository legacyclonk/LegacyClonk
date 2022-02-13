const fs = require('fs');

// The following array will contain all C4Script functions documented in the lcdocs.
const lcdocsFunctions = filterFile('.github/workflows/C4ScriptDocAnalyzer/lcdocs_functions.txt', /^Name=(\w*)/gm, 1);
console.log(`Loaded generated file with C4Script functions documented in the current lcdocs master https://github.com/legacyclonk/lcdocs`);

// The following array will contain all C4Script functions defined in C4Script.cpp.
// Format example (is a bunch of [Function]-stanzas):
// [Function]
// Name=AbortMessageBoard
// Return=bool
// Parameter=object pObj, int iPlr
// DescDE=aufgerufene Eingabezeile.
const c4ScriptFunctions = filterFile('./src/C4Script.cpp', /AddFunc\(pEngine, +"(\w+)", +Fn\w+(, +(false|true))?\);/gm, 1)
console.log(`Loaded C4Script.cpp`);

// Searching the whole directory for files of filetype ".c".
// All matching files are read in and filtered for function names in the ".c"-files that are not excluded with "// internal" in the line before the function declaration.
const systemC4gDirectory = 'planet/System.c4g/';
const systemC4gFileNames = fs.readdirSync(systemC4gDirectory, {
    "encoding": "ascii",
    "withFileTypes": false
});
const cFileNames = systemC4gFileNames.filter(__filename => __filename.endsWith('.c'));
const cFunctions = cFileNames.map(
    __filename => filterFile(systemC4gDirectory.concat(__filename), /(?<!\/\/\W?internal\r?\n)global func (\w*)\(/gm, 1)
);
console.log(`Loaded helper files from System.c4g`);

console.log(`\nThe following C4Script functions are defined in the engine but not documented in the current lcdocs master:\n`);
let undocumentedFunctions = 0;
// Adding functions from C4Script.cpp to functions from .c-files in System.c4g.
// This array of arrays will be searched for negative comparison results with the lcdocs.
const engineFunctions = [c4ScriptFunctions].concat(cFunctions);
const engineFileNames = ['C4Script.cpp'].concat(cFileNames);

engineFunctions.forEach((functionsFile, engineFunctionsIndex) => {
    functionsFile.forEach((c4ScriptFunctionName) => {
        if (!lcdocsFunctions.includes(c4ScriptFunctionName)) {
            console.log(`\tDefined in ${engineFileNames[engineFunctionsIndex]}: ${c4ScriptFunctionName}()`);
            undocumentedFunctions++;
            process.exitCode = 1;
        }
    });
});

console.log(`\nThe following C4Script functions are documented in the current lcdocs master but not defined in the engine:\n`);
let undefinedFunctions = 0;
lcdocsFunctions.forEach(c4ScriptFunctionName => {
    if (!engineFunctions.flat().includes(c4ScriptFunctionName)) {
        console.log(`\t${c4ScriptFunctionName}()`);
        undefinedFunctions++;
    }
});

console.log(`\nThere are\n * ${c4ScriptFunctions.length} defined functions in C4Script.cpp,\n * ${cFunctions.flat().length} defined functions in System.c4g and\n * ${lcdocsFunctions.length} documented functions in lcdocs.\n`);
console.log(`Exiting with ${undocumentedFunctions} undocumented functions and ${undefinedFunctions} undefined functions.`);


function filterFile(filepath, regex, captureGroup = 1) {
    console.log(`Loading ${filepath.replace(/^.*[\\\/]/, '')}`);
    const file = fs.readFileSync(filepath).toString();
    let functionMatcher = regex.exec(file);
    const functions = [];

    do {
        functions.push(functionMatcher[captureGroup]);
    } while ((functionMatcher = regex.exec(file)) !== null);

    return functions;
}