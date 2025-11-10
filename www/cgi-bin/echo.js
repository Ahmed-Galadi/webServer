#!/usr/bin/env node

const method = process.ev.REQUEST_METHOD || 'GET';
const query = process.env.QUERY_STRING || '';

//console.log('Status: 200 OK');
console.log('Content-Type: text/html\n');
console.log('<html>');
console.log('<head><title>Echo Test</title></head>');
console.log('<body>');
console.log('<h1>CGI Echo Test</h1>');
console.log('<p>Method: ' + method + '</p>');
console.log('<p>Query String: ' + query + '</p>');
console.log('<hr>');
console.log('<h2>Test Links:</h2>');
console.log('<ul>');
console.log('<li><a href="/cgi-bin/echo.js">Simple GET</a></li>');
console.log('<li><a href="/cgi-bin/echo.js?name=alice&age=25">GET with params</a></li>');
console.log('<li><a href="/cgi-bin/test.php">PHP Test</a></li>');
console.log('<li><a href="/cgi-bin/test.py">Python Test</a></li>');
console.log('</ul>');
console.log('<form method="POST" action="/cgi-bin/echo.js">');
console.log('<input type="text" name="data" placeholder="Enter data">');
console.log('<button type="submit">Submit POST</button>');
console.log('</form>');
console.log('</body>');
console.log('</html>');


// const method = process.env.REQUEST_METHOD;
// const queryString = process.env.QUERY_STRING || '';
// const contentType = process.env.CONTENT_TYPE || '';
// const contentLength = parseInt(process.env.CONTENT_LENGTH) || 0;

// let body = '';

// const { spawn } = require('child_process');

// const ls = spawn('/bin/node', ['/home/bzinedda/Desktop/ahmed/www/cgi-bin/echo.js']);
// ls.stdout.on('data', (data) => {
//   console.log(`stdout: ${data}`);
// });

// // Read stdin
// process.stdin.setEncoding('utf8');
// process.stdin.on('data', chunk => {
//     body += chunk;
// });

// process.stdin.on('end', () => {
//     // Output CGI response (Status header is required)
//     console.log('Status: 200 OK');
//     console.log('Content-Type: application/json');
//     console.log('');
    
//     console.log(JSON.stringify({
//         method: method,
//         query: queryString,
//         contentType: contentType,
//         contentLength: contentLength,
//         body: body.substring(0, 200),
//         timestamp: new Date().toISOString()
//     }, null, 2));
// });
