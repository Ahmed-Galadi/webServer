#!/usr/bin/env node

// Simple Node.js CGI form handler

const querystring = require("querystring");

// Helper to print HTTP headers and HTML safely
function printHeader() {
  console.log("Content-Type: text/html; charset=utf-8\n");
}

function htmlTemplate(body) {
  return `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>CGI Form Example</title>
<style>
  body { 
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: linear-gradient(135deg, #1c1e26 0%, #2a2d39 100%);
    color: #f0f2f5;
    padding: 40px; 
    display: flex;
    justify-content: center;
    align-items: center;
    min-height: 100vh;
  }
  .container {
    background: #252831;
    border-radius: 15px;
    padding: 50px;
    max-width: 600px;
    width: 100%;
    box-shadow: 0 20px 60px rgba(0,0,0,0.3);
  }
  h1 { color: #61dafb; }
  form { background: #252831; padding: 20px; border-radius: 10px; max-width: 500px; }
  label { display: block; margin-top: 12px; color: #a7b0c0; }
  input, textarea { 
    width: 100%; 
    padding: 12px; 
    margin-top: 4px; 
    border: 2px solid #444; 
    border-radius: 8px; 
    background: #1c1e26;
    color: #f0f2f5;
    font-size: 16px;
  }
  input:focus, textarea:focus {
      outline: none;
      border-color: #61dafb;
  }
  input[type="submit"] { 
    background: #61dafb; 
    color: #1c1e26; 
    border: none; 
    cursor: pointer; 
    margin-top: 16px; 
    font-weight: bold;
    padding: 15px;
  }
  input[type="submit"]:hover { background: #88f0ff; }
  .data-box { background: #1c1e26; border-left: 4px solid #61dafb; border-radius: 8px; padding: 20px; margin-top: 20px; }
  .back-link { text-align: center; margin-top: 20px; }
  .back-link a { color: #61dafb; text-decoration: none; }
</style>
</head>
<body>
  <div class="container">
    ${body}
    <div class="back-link"><a href="/">← Back to Home</a></div>
  </div>
</body>
</html>`;
}

// The form HTML
function formHTML() {
  return `
  <h1>Submit your info</h1>
  <form method="POST" action="/cgi-bin/form.js">
    <label>Full name:
      <input type="text" name="name" required>
    </label>
    <label>Email:
      <input type="email" name="email" required>
    </label>
    <label>Message:
      <textarea name="message" rows="4" required></textarea>
    </label>
    <input type="submit" value="Send">
  </form>`;
}

// Handle incoming POST data
function handlePostData(callback) {
  let body = "";
  process.stdin.on("data", chunk => (body += chunk));
  process.stdin.on("end", () => {
    const data = querystring.parse(body);
    callback(data);
  });
}

// Main logic
const method = process.env.REQUEST_METHOD || "GET";

if (method === "POST") {
  handlePostData(data => {
    printHeader();
    const output = `
      <h1>Thank you!</h1>
      <p>Here’s what you submitted:</p>
      <div class="data-box">
        <p><b>Name:</b> ${data.name || ""}</p>
        <p><b>Email:</b> ${data.email || ""}</p>
        <p><b>Message:</b> ${data.message || ""}</p>
      </div>
      <hr>
      <p>Want to fill it again?</p>
      ${formHTML()}
    `;
    console.log(htmlTemplate(output));
  });
} else {
  printHeader();
  console.log(htmlTemplate(formHTML()));
}
