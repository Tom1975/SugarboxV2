'use strict';

const path = require('path');

module.exports = {
    target: 'node',
    mode: 'none',
    entry: './out/main.js',
    output: {
        path: path.resolve(__dirname, 'dist'),
        filename: 'main.js',
        libraryTarget: 'commonjs2'
    },
    externals: {
        vscode: 'commonjs vscode'
    },
    resolve: {
        extensions: ['.js']
    },
    // Source maps for debugging in the Extension Dev Host
    devtool: 'source-map'
};
