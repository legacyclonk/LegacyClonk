const fs = require('fs');

// The following array will contain all C4Script functions documented in the lcdocs.
// Format example (is a bunch of [Function]-stanzas):
// [Function]
// Name=AbortMessageBoard
// Return=bool
// Parameter=object pObj, int iPlr
// DescDE=aufgerufene Eingabezeile.
const lcdocsFunctions = readAndFilterFile('.github/workflows/C4ScriptDocAnalyzer/lcdocs_functions.txt', [
    {
        regex: /^Name=(\w*)/gm,
        captureGroup: 1,
    }
]);
console.log(`Loaded generated file with C4Script functions documented in the current lcdocs master https://github.com/legacyclonk/lcdocs`);

// The following array will contain all C4Script functions defined in C4Script.cpp.
const c4ScriptFunctions = readAndFilterFile('./src/C4Script.cpp', [
    {
        regex: /AddFunc\(pEngine, +"(\w+)", +Fn\w+(, +(false|true))?\);/gm,
        captureGroup: 1,
    },
    {
        regex: /new\sC4Aul\w+<\w+,\s?\w+>\s*{pEngine,\s"(\w+)"/gm,
        captureGroup: 1,
    }
]);
console.log(`Loaded C4Script.cpp functions`);

const c4ScriptConstants = readAndFilterFile('./src/C4Script.cpp', [
    {
        regex: /^\t{\s?"(\w+)",\s*\w+,\s*[\w<>():]+\s*}(,|\r?\n};)(\s*\/\/(\s*[\w()/-;,]+)+)?/gm,
        captureGroup: 1,
    }
]);
console.log(`Loaded C4Script.cpp constants`);


// Searching the whole directory for files of filetype ".c".
// All matching files are read in and filtered for function names in the ".c"-files that are not excluded with "// internal" in the line before the function declaration.
const systemC4gDirectory = 'planet/System.c4g/';
const systemC4gFileNames = fs.readdirSync(systemC4gDirectory, {
    "encoding": "ascii",
    "withFileTypes": false
});
const cFileNames = systemC4gFileNames.filter(__filename => __filename.endsWith('.c'));
const cFunctions = cFileNames.map(
    __filename => removeOverloadingFunctions(
        readAndFilterFile(systemC4gDirectory.concat(__filename),[
    {
        regex: /(?<!\/\/\s?internal\r?\n)global func (\w*)\s?\(/gm,
        captureGroup: 1,
    }
    ]), c4ScriptFunctions)
);
console.log(`Loaded helper files from System.c4g`);

// Adding functions from C4Script.cpp to functions from .c-files in System.c4g.
// This array of arrays will be searched for negative comparison results with the lcdocs.
const engineFunctions = [c4ScriptFunctions].concat(cFunctions);
const engineFileNames = ['C4Script.cpp'].concat(cFileNames);

console.log(`\nThe following C4Script functions are defined in the engine but not documented in the current lcdocs master:\n`);
const undocumentedFunctions = findUndocumented(engineFunctions, lcdocsFunctions, '()');

console.log(`\nThe following C4Script constants are defined in the engine but not documented in the current lcdocs master:\n`);
const undocumentedConstants = findUndocumented([c4ScriptConstants], lcdocsFunctions, '');

console.log(`\nThe following C4Script functions and constants are documented in the current lcdocs master but not defined in the engine:\n`);
const undefinedFunctions = findUndefined(engineFunctions.concat([c4ScriptConstants]), lcdocsFunctions, '()');

let colorUndocumentedFunc = '\x1b[42m\x1b[30m';
let colorUndocumentedConst = '\x1b[42m\x1b[30m';
let colorUndefinedFunc = '\x1b[42m\x1b[30m';
if (undocumentedFunctions !== 0) colorUndocumentedFunc = '\x1b[41m\x1b[30m';
if (undocumentedConstants !== 0) colorUndocumentedConst = '\x1b[41m\x1b[30m';
if (undefinedFunctions !== 0) colorUndefinedFunc = '\x1b[43m\x1b[30m';
console.log(`\n * ${colorUndocumentedFunc}${undocumentedFunctions} functions are undocumented.\x1b[0m`);
console.log(` * ${colorUndocumentedConst}${undocumentedConstants} constants are undocumented.\x1b[0m`);
console.log(` * ${colorUndefinedFunc}${undefinedFunctions} functions or constants are documented but undefined in the engine.\x1b[0m`);
console.log(`\nStatistics: There are
 * ${c4ScriptFunctions.length} defined functions in C4Script.cpp,
 * ${c4ScriptConstants.length} defined constants in C4Script.cpp,
 * ${cFunctions.flat().length} defined functions in System.c4g and
 * ${lcdocsFunctions.length} documented functions and constants in current lcdocs master.\n`);

const sumEntities = c4ScriptFunctions.length + c4ScriptConstants.length + cFunctions.flat().length;
const sumProcessed = lcdocsFunctions.length - undefinedFunctions + undocumentedFunctions + undocumentedConstants;
if (sumEntities === sumProcessed) {
    console.log(`All entities processed, exiting.`);
    process.exit();
} else {
    console.log(`\x1b[41m\x1b[30mERROR: The sum of functions and constants is ${sumEntities} but ${sumProcessed} where processed.
    Make sure that the set of parsed functions is distinct!\x1b[0m`);
    process.exit(2)
}

function readAndFilterFile(filepath, captures) {
    console.log(`Loading ${filepath.replace(/^.*[\\\/]/, '')}`);

    const file = fs.readFileSync(filepath).toString();
    const functions = [];
    captures.forEach(capture => {
        let functionMatcher = capture.regex.exec(file);
                do {
            functions.push(functionMatcher[capture.captureGroup]);
        } while ((functionMatcher = capture.regex.exec(file)) !== null);
    })

    return functions;
}

// Abstract function to find functions and constants separated. entitySuffix can be "()" when used with function names.
function findUndocumented(engineEntities, lcdocsEntities, entitySuffix) {
    let undocumentedEntities = 0;
    engineEntities.forEach((entityFile, engineEntitiesIndex) => {
        entityFile.forEach((c4ScriptEntityName) => {
            if (!lcdocsEntities.includes(c4ScriptEntityName)) {
                console.log(`\tDefined in ${engineFileNames[engineEntitiesIndex]}: ${c4ScriptEntityName}${entitySuffix}`);
                undocumentedEntities++;
                process.exitCode = 1;
            }
        });
    });
    return undocumentedEntities;
}

// Abstract function to find constants and functions separated. entitySuffix can be "()" when used with function names.
function findUndefined(engineEntities, lcdocsEntities, entitySuffix) {
    let undefinedEntities = 0;
    lcdocsEntities.forEach(c4ScriptEntityName => {
        if (!engineEntities.flat().includes(c4ScriptEntityName)) {
            console.log(`\t${c4ScriptEntityName}${entitySuffix}`);
            undefinedEntities++;
        }
    });
    return undefinedEntities;
}

// ".c"-files can contain functions or constants that overload C4Script.cpp. This function removes duplicates in an array.
function removeOverloadingFunctions(array, c4ScriptCppEntities) {
    let uniqueArray = [];
    array.forEach((element) => {
        if (!c4ScriptCppEntities.includes(element)) {
            uniqueArray.push(element);
        }
    });
    return uniqueArray;
}