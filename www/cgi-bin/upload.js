#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

const method = process.env.REQUEST_METHOD;
const contentType = process.env.CONTENT_TYPE || '';
const uploadDir = path.resolve(__dirname, '../upload');

if (!fs.existsSync(uploadDir)) {
    fs.mkdirSync(uploadDir, { recursive: true });
}

let body = Buffer.alloc(0);
process.stdin.on('data', chunk => {
    body = Buffer.concat([body, Buffer.from(chunk, 'binary')]);
});

process.stdin.on('end', () => {
    if (!contentType.startsWith('multipart/form-data')) {
        console.log('Status: 400 Bad Request');
        console.log('Content-Type: text/plain\n');
        console.log('Expected multipart/form-data');
        return;
    }

    // Extract boundary
    const boundaryMatch = contentType.match(/boundary=(.+)$/);
    if (!boundaryMatch) {
        console.log('Status: 400 Bad Request');
        console.log('Content-Type: text/plain\n');
        console.log('Missing boundary');
        return;
    }

    const boundary = '--' + boundaryMatch[1];
    const parts = body.toString('binary').split(boundary);

    parts.forEach(part => {
        // Skip empty parts
        if (!part || part === '--\r\n') return;

        // Find filename
        const filenameMatch = part.match(/filename="(.+?)"/);
        if (filenameMatch) {
            const filename = path.basename(filenameMatch[1]);

            // Extract file content (after double CRLF)
            const contentIndex = part.indexOf('\r\n\r\n');
            if (contentIndex !== -1) {
                const fileContent = part.slice(contentIndex + 4, part.length - 2); // remove ending \r\n
                const filePath = path.join(uploadDir, filename);

                fs.writeFileSync(filePath, fileContent, 'binary');
                console.log(`[DEBUG] Saved uploaded file: ${filePath}`);
            }
        }
    });

    // Respond JSON
    console.log('Status: 200 OK');
    console.log('Content-Type: application/json\n');
    console.log(JSON.stringify({
        success: true,
        received: method,
        size: body.length,
        uploadDir: uploadDir,
        timestamp: new Date().toISOString()
    }, null, 2));
});
