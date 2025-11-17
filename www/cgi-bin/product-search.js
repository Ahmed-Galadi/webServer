#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const querystring = require('querystring');

const productsDB = path.resolve(__dirname, '../products.json');

function escapeHtml(str) {
  if (!str) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

function renderPage(body) {
    return `Content-Type: text/html\n\n<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Product Search</title>
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
            max-width: 800px;
            width: 100%;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
        }
        h1 {
            color: #61dafb;
            text-align: center;
            margin-bottom: 30px;
        }
        form { display: flex; gap: 10px; margin-bottom: 30px; }
        input[type="text"] {
            flex: 1; padding: 15px; border: 2px solid #444; border-radius: 8px; font-size: 16px; background: #1c1e26; color: #f0f2f5;
        }
        input[type="text"]:focus { outline: none; border-color: #61dafb; }
        .search-btn { background: #61dafb; color: #1c1e26; border: none; padding: 15px 30px; border-radius: 8px; cursor: pointer; font-weight: bold; }
        .results { margin-top: 20px; }
        .product { background: #1c1e26; padding: 20px; border-radius: 8px; margin-bottom: 15px; border-left: 4px solid #61dafb; }
        .product h3 { color: #61dafb; margin-bottom: 5px; }
        .product p { color: #a7b0c0; }
        .product .price { font-weight: bold; color: #f0f2f5; }
        .no-results { text-align: center; color: #a7b0c0; }
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

function renderSearchForm(searchTerm = '') {
    return `
        <h1>Product Search</h1>
        <form action="/cgi-bin/product-search.js" method="GET">
            <input type="text" name="search" placeholder="Search for products..." value="${escapeHtml(searchTerm)}">
            <button type="submit" class="search-btn">Search</button>
        </form>
    `;
}

function renderResults(searchTerm, results) {
    let resultsHtml = '<div class="results">';
    if (results.length > 0) {
        results.forEach(product => {
            resultsHtml += `
                <div class="product">
                    <h3>${escapeHtml(product.name)}</h3>
                    <p>${escapeHtml(product.description)}</p>
                    <p class="price">$${product.price}</p>
                </div>
            `;
        });
    } else {
        if (searchTerm) {
            resultsHtml += `<p class="no-results">No products found for "${escapeHtml(searchTerm)}".</p>`;
        } else {
            resultsHtml += `<p class="no-results">No products available.</p>`;
        }
    }
    resultsHtml += '</div>';
    return resultsHtml;
}

const method = process.env.REQUEST_METHOD || 'GET';

if (method === 'GET') {
    const url = require('url');
    const query = url.parse(process.env.REQUEST_URI, true).query;
    const searchTerm = (query.search || '').trim();
    
    const products = JSON.parse(fs.readFileSync(productsDB, 'utf8'));
    let results = products;

    if (searchTerm) {
        results = products.filter(p => 
            p.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
            p.description.toLowerCase().includes(searchTerm.toLowerCase())
        );
    }
    
    let body = renderSearchForm(searchTerm) + renderResults(searchTerm, results);
    
    process.stdout.write(renderPage(body));
    process.exit(0);
}
