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
  body { font-family: sans-serif; background: #f2f6fb; padding: 40px; }
  h1 { color: #1d7ed8; }
  form { background: white; padding: 20px; border-radius: 10px; max-width: 500px; box-shadow: 0 4px 20px rgba(0,0,0,0.1); }
  label { display: block; margin-top: 12px; }
  input, textarea { width: 100%; padding: 8px; margin-top: 4px; border: 1px solid #ccc; border-radius: 6px; }
  input[type="submit"] { background: #1d7ed8; color: white; border: none; cursor: pointer; margin-top: 16px; }
  input[type="submit"]:hover { background: #0d63b8; }
  .data-box { background: #eaf4ff; border-radius: 8px; padding: 10px; margin-top: 20px; }
</style>
</head>
<body>
  ${body}
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
