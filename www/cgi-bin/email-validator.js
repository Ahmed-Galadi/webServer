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
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 15px;
            padding: 50px;
            max-width: 500px;
            width: 100%;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
        }
        h1 {
            color: #667eea;
            text-align: center;
            margin-bottom: 30px;
        }
        form { display: flex; gap: 10px; }
        input[type="text"] {
            flex: 1; padding: 15px; border: 2px solid #ddd; border-radius: 8px; font-size: 16px;
        }
        input[type="text"]:focus { outline: none; border-color: #667eea; }
        .validate-btn { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border: none; padding: 15px 30px; border-radius: 8px; cursor: pointer; }
        .result { padding: 15px; border-radius: 8px; margin-top: 20px; text-align: center; font-weight: bold; }
        .result.valid { background: #d4edda; color: #155724; border: 2px solid #28a745; }
        .result.invalid { background: #f8d7da; color: #721c24; border: 2px solid #dc3545; }
        .back-link { text-align: center; margin-top: 20px; }
        .back-link a { color: #667eea; text-decoration: none; }
    </style>
</head>
<body>
    <div class="container">
        <h1>✉️ Email Validator</h1>
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
  process.stdout.write(renderPage('', '', ''));
  process.exit(0);
}

let body = '';
process.stdin.setEncoding('utf8');
process.stdin.on('data', chunk => { body += chunk; });
process.stdin.on('end', () => {
  try {
    const params = qs.parse(body || '');
    const email = (params.email || '').trim();
    const emailRegex = /^[a-zA-Z0-9._-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/;
    if (!email) {
      process.stdout.write(renderPage('', 'invalid', '✗ Please provide an email'));
      process.exit(0);
    }
    if (emailRegex.test(email)) {
      process.stdout.write(renderPage(email, 'valid', '✓ Valid email format'));
    } else {
      process.stdout.write(renderPage(email, 'invalid', '✗ Invalid email format'));
    }
  } catch (err) {
    process.stdout.write(renderPage('', 'invalid', '✗ Error processing request'));
  }
});
