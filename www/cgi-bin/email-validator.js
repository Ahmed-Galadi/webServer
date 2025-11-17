#!/usr/bin/env node

const method = (process.env.REQUEST_METHOD || 'GET').toUpperCase();
const qs = require('querystring');

function escapeHtml(str) {
  if (!str) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

function renderPage(emailValue, resultClass, resultMessage) {
  const emailEsc = escapeHtml(emailValue || '');
  const resultHtml = resultMessage
    ? `<div id="result" class="result ${resultClass}">${escapeHtml(resultMessage)}</div>`
    : '<div id="result" class="result" style="display:none"></div>';

  return `Content-Type: text/html\n\n<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Email Validator</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1c1e26 0%, #2a2d39 100%);
            color: #f0f2f5;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: #252831;
            border-radius: 15px;
            padding: 50px;
            max-width: 500px;
            width: 100%;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
        }
        h1 {
            color: #61dafb;
            text-align: center;
            margin-bottom: 30px;
        }
        form { display: flex; gap: 10px; }
        input[type="text"] {
            flex: 1; padding: 15px; border: 2px solid #444; border-radius: 8px; font-size: 16px; background: #1c1e26; color: #f0f2f5;
        }
        input[type="text"]:focus { outline: none; border-color: #61dafb; }
        .validate-btn { background: #61dafb; color: #1c1e26; border: none; padding: 15px 30px; border-radius: 8px; cursor: pointer; font-weight: bold; }
        .result { padding: 15px; border-radius: 8px; margin-top: 20px; text-align: center; font-weight: bold; }
        .result.valid { background: #1c3b33; color: #a6e9d5; border: 2px solid #2ca880; }
        .result.invalid { background: #4d2028; color: #f8b4c0; border: 2px solid #d9304f; }
        .back-link { text-align: center; margin-top: 20px; }
        .back-link a { color: #61dafb; text-decoration: none; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Email Validator</h1>
        <form action="/cgi-bin/email-validator.js" method="POST">
            <input type="text" name="email" id="emailInput" placeholder="Enter email address" value="${emailEsc}" required>
            <button type="submit" class="validate-btn">Validate</button>
        </form>
        ${resultHtml}
        <div class="back-link"><a href="/">← Back to Home</a></div>
    </div>
</body>
</html>`;
}

if (method === 'GET') {
    const url = require('url');
    const query = url.parse(process.env.REQUEST_URI, true).query;
    const email = query.email || '';
    const result = query.result || '';
    let resultClass = '';
    let resultMessage = '';

    if (result === 'valid') {
        resultClass = 'valid';
        resultMessage = 'Valid email format';
    } else if (result === 'invalid') {
        resultClass = 'invalid';
        resultMessage = 'Invalid email format';
    } else if (result === 'empty') {
        resultClass = 'invalid';
        resultMessage = 'Please provide an email';
    }

    process.stdout.write(renderPage(email, resultClass, resultMessage));
    process.exit(0);
}

if (method === 'POST') {
    let body = '';
    process.stdin.setEncoding('utf8');
    process.stdin.on('data', chunk => { body += chunk; });
    process.stdin.on('end', () => {
        try {
            const params = qs.parse(body || '');
            const email = (params.email || '').trim();
            const emailRegex = /^[a-zA-Z0-9._-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/;
            
            let result = '';
            if (!email) {
                result = 'empty';
            } else if (emailRegex.test(email)) {
                result = 'valid';
            } else {
                result = 'invalid';
            }
            
            // Redirect
            const redirectUrl = `/cgi-bin/email-validator.js?result=${result}&email=${encodeURIComponent(email)}`;
            process.stdout.write(`Status: 303 See Other\n`);
            process.stdout.write(`Location: ${redirectUrl}\n\n`);

        } catch (err) {
            process.stdout.write(`Status: 303 See Other\n`);
            process.stdout.write(`Location: /cgi-bin/email-validator.js?result=error\n\n`);
        }
    });
}
