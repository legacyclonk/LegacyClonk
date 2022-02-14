const fs = require('fs');

// The following array will contain all C4Script functions documented in the lcdocs.
// Format example (is a bunch of [Function]-stanzas):
// [Function]
// Name=AbortMessageBoard
// Return=bool
// Parameter=object pObj, int iPlr
// DescDE=aufgerufene Eingabezeile.
const lcdocsFunctions = filterFile('.github/workflows/C4ScriptDocAnalyzer/lcdocs_functions.txt', [
    {
        regex: /^Name=(\w*)/gm,
        captureGroup: 1,
    }
]);
console.log(`Loaded generated file with C4Script functions documented in the current lcdocs master https://github.com/legacyclonk/lcdocs`);

// The following array will contain all C4Script functions defined in C4Script.cpp.
const c4ScriptFunctions = filterFile('./src/C4Script.cpp', [
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

const c4ScriptConstants = filterFile('./src/C4Script.cpp', [
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
    __filename => filterFile(systemC4gDirectory.concat(__filename),[
    {
        regex: /(?<!\/\/\W?internal\r?\n)global func (\w*)\(/gm,
        captureGroup: 1,
    }
    ])
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

console.log(`\nThere are\n * ${c4ScriptFunctions.length} defined functions in C4Script.cpp,\n * ${c4ScriptConstants.length} defined constants in C4Script.cpp,\n * ${cFunctions.flat().length} defined functions in System.c4g and\n * ${lcdocsFunctions.length} documented functions and constants in lcdocs.\n`);
console.log(`Exiting with ${undocumentedFunctions} undocumented functions, ${undocumentedConstants} undocumented constants and ${undefinedFunctions} undefined functions or constants.`);


function filterFile(filepath, captures) {
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