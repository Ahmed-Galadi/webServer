#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import cgi
import cgitb
import json
import random
import sys

# Enable CGI error reporting
cgitb.enable()

def generate_cards():
    """Generate 8 pairs (16 cards) of simple SVG shapes for a 4x4 grid."""
    base_shapes = [
        '<circle cx="40" cy="40" r="30" fill="none" stroke="white" stroke-width="3"/>',
        '<rect x="20" y="20" width="40" height="40" fill="none" stroke="white" stroke-width="3"/>',
        '<polygon points="40,15 65,65 15,65" fill="none" stroke="white" stroke-width="3"/>',
        '<polygon points="40,10 70,40 40,70 10,40" fill="white"/>',
        '<polygon points="40,10 60,25 60,55 40,70 20,55 20,25" fill="white"/>',
        '<circle cx="40" cy="40" r="25" fill="white"/>',
        '<rect x="25" y="25" width="30" height="30" fill="white"/>',
        '<polygon points="40,10 45,30 65,30 50,42 55,62 40,50 25,62 30,42 15,30 35,30" fill="white"/>',
        # --- We only need 8 shapes for a 4x4 grid ---
        # The rest of the original 18 shapes are not needed.
    ]
    
    shapes = []
    # --- CHANGE 1: Use 8 shapes, not 18 ---
    for s in base_shapes[:8]:
        shapes.append(s)
        shapes.append(s)
        
    random.shuffle(shapes)
    return shapes

def star_threshold(moves):
    """
    Adjusted star ratings for a 4x4 game.
    (Note: This Python function isn't used by the client-side JS,
    but it's good practice to keep it consistent.)
    """
    # --- CHANGE 2: Adjust star thresholds for a 4x4 game ---
    if moves <= 12: # Was 30
        return 3
    if moves <= 18: # Was 45
        return 2
    if moves <= 25: # Was 60
        return 1
    return 0

def main():
    # Print the HTTP header
    print("Content-Type: text/html\n")
    
    # Get card data
    cards = generate_cards()
    cards_json = json.dumps(cards)
    
    # Define the HTML template
    html = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Memory Game (4x4)</title>
    <style>
        body {
            font-family: 'Arial', sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            background: #0f0c29;
            background: linear-gradient(to right, #24243e, #302b63, #0f0c29);
            color: #e0e0e0;
            margin: 0;
            padding: 20px;
            box-sizing: border-box;
        }
        
        .header {
            width: 100%;
            max-width: 400px; /* Adjusted max-width for 4x4 */
            text-align: center;
            margin-bottom: 20px;
        }
        
        h1 {
            color: #00d4ff;
            text-shadow: 0 0 10px rgba(0, 212, 255, 0.5);
            margin-bottom: 20px;
        }
        
        .stats {
            display: flex;
            justify-content: space-around;
            background: rgba(0, 0, 0, 0.2);
            padding: 10px 20px;
            border-radius: 10px;
            border: 2px solid #00d4ff;
        }
        
        .stat-value {
            font-weight: bold;
            color: #00d4ff;
            font-size: 1.5em;
        }
        
        .stars {
            font-size: 2em;
            margin-top: 5px;
        }
        
        .star {
            color: #333;
            margin: 0 2px;
        }
        
        .star.active {
            color: #00d4ff;
            text-shadow: 0 0 10px rgba(0, 212, 255, 0.8);
        }
        
        .game-board {
            display: grid;
            /* --- CHANGE 3: Grid from 6x6 to 4x4 --- */
            grid-template-columns: repeat(4, 80px);
            grid-template-rows: repeat(4, 80px);
            gap: 10px;
            margin-bottom: 20px;
            padding: 20px;
            background: rgba(0, 0, 0, 0.3);
            border-radius: 15px;
            border: 2px solid #00d4ff;
        }
        
        .card {
            width: 80px;
            height: 80px;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            border: 2px solid #00d4ff;
            border-radius: 8px;
            cursor: pointer;
            position: relative;
            transform-style: preserve-3d;
            transition: transform 0.3s, box-shadow 0.3s;
        }
        
        .card:hover:not(.flipped):not(.matched) {
            transform: scale(1.05);
            box-shadow: 0 0 15px rgba(0, 212, 255, 0.5);
        }
        
        .card.flipped, .card.matched {
            background: linear-gradient(135deg, #0066cc 0%, #004999 100%);
            border-color: #00d4ff;
        }
        
        .card.matched {
            background: linear-gradient(135deg, #00d4ff 0%, #0099cc 100%);
            cursor: default;
            animation: matchPulse 0.5s ease;
        }
        
        @keyframes matchPulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.1); }
        }
        
        .card-front, .card-back {
            position: absolute;
            width: 100%;
            height: 100%;
            display: flex;
            align-items: center;
            justify-content: center;
            border-radius: 6px;
        }
        
        .card-back {
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
        }
        
        .card-back::before {
            content: '?';
            font-size: 2em;
            color: #00d4ff;
            font-weight: bold;
        }
        
        .card-front {
            display: none;
        }
        
        .card.flipped .card-front,
        .card.matched .card-front {
            display: flex;
        }
        
        .card.flipped .card-back,
        .card.matched .card-back {
            display: none;
        }
        
        .card-front svg {
            width: 80%;
            height: 80%;
        }
        
        .controls {
            display: flex;
            gap: 15px;
        }
        
        button {
            background: linear-gradient(135deg, #00d4ff 0%, #0099cc 100%);
            color: white;
            border: none;
            padding: 12px 30px;
            font-size: 1.1em;
            border-radius: 8px;
            cursor: pointer;
            font-weight: bold;
            transition: all 0.3s;
            box-shadow: 0 4px 15px rgba(0, 212, 255, 0.3);
        }
        
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(0, 212, 255, 0.5);
        }
        
        button:active {
            transform: translateY(0);
        }
        
        .win-message {
            position: fixed;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%) scale(0);
            background: linear-gradient(135deg, #16213e 0%, #1a1a2e 100%);
            padding: 40px;
            border-radius: 20px;
            border: 3px solid #00d4ff;
            text-align: center;
            display: none;
            z-index: 1000;
            box-shadow: 0 0 50px rgba(0, 212, 255, 0.5);
        }
        
        .win-message.show {
            display: block;
            /* Keep final scale after animation to prevent disappearing */
            transform: translate(-50%, -50%) scale(1);
            animation: popIn 0.5s ease forwards;
        }
        
        @keyframes popIn {
            0% { transform: translate(-50%, -50%) scale(0); }
            100% { transform: translate(-50%, -50%) scale(1); }
        }
        
        .win-message h2 {
            font-size: 2.5em;
            color: #00d4ff;
            margin-bottom: 20px;
            text-shadow: 0 0 10px rgba(0, 212, 255, 0.5);
        }
        
        .win-message .final-stats {
            font-size: 1.3em;
            margin: 20px 0;
        }
        
        .overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0, 0, 0, 0.8);
            display: none;
            z-index: 999;
        }
        
        .overlay.show {
            display: block;
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>Memory Game</h1>
        <div class="stats">
            <div class="stat">
                <div>Moves</div>
                <div class="stat-value" id="moves">0</div>
            </div>
            <div class="stat">
                <div>Pairs Found</div>
                <div class="stat-value" id="pairs">0 / 8</div>
            </div>
            <div class="stat">
                <div>Stars</div>
                <div class="stars" id="stars">
                    <span class="star">★</span>
                    <span class="star">★</span>
                    <span class="star">★</span>
                </div>
            </div>
        </div>
    </div>
    
    <div class="game-board" id="gameBoard"></div>
    
    <div class="controls">
        <button onclick="resetGame()">New Game</button>
    </div>
    
    <div class="overlay" id="overlay"></div>
    <div class="win-message" id="winMessage">
        <h2>Congratulations!</h2>
        <div class="final-stats">
            <p>You completed the game in <strong id="finalMoves">0</strong> moves!</p>
            <div class="stars" id="finalStars" style="font-size: 3em; margin: 20px 0;">
                <span class="star">★</span>
                <span class="star">★</span>
                <span class="star">★</span>
            </div>
        </div>
        <button type="button" onclick="resetGame()">Close</button>
    </div>
    
    <script>
        // --- CHANGE 5: Inject the JSON data correctly ---
        const cards = __CARDS_JSON__; 
        
        let flippedCards = [];
        let matchedPairs = 0;
        let moves = 0;
        let canFlip = true;
        
        function initGame() {
            const board = document.getElementById('gameBoard');
            board.innerHTML = '';
            
            cards.forEach((shape, index) => {
                const card = document.createElement('div');
                card.className = 'card';
                card.dataset.index = index;
                card.dataset.shape = shape;
                
                card.innerHTML = 
                    '<div class="card-back"></div>' +
                    '<div class="card-front">' +
                        '<svg viewBox="0 0 80 80" xmlns="http://www.w3.org/2000/svg">' +
                            shape +
                        '</svg>' +
                    '</div>';
                
                card.addEventListener('click', () => flipCard(card));
                board.appendChild(card);
            });
            
            moves = 0;
            matchedPairs = 0;
            updateStats();
        }
        
        function flipCard(card) {
            if (!canFlip || card.classList.contains('flipped') || card.classList.contains('matched')) {
                return;
            }
            
            card.classList.add('flipped');
            flippedCards.push(card);
            
            if (flippedCards.length === 2) {
                canFlip = false;
                moves++;
                updateStats();
                
                setTimeout(checkMatch, 800);
            }
        }
        
        function checkMatch() {
            const [card1, card2] = flippedCards;
            
            if (card1.dataset.shape === card2.dataset.shape) {
                card1.classList.add('matched');
                card2.classList.add('matched');
                card1.classList.remove('flipped');
                card2.classList.remove('flipped');
                matchedPairs++;
                updateStats();
                
                // --- CHANGE 6: Check for 8 pairs, not 18 ---
                if (matchedPairs === 8) {
                    setTimeout(showWinMessage, 500);
                }
            } else {
                card1.classList.remove('flipped');
                card2.classList.remove('flipped');
            }
            
            flippedCards = [];
            canFlip = true;
        }
        
        function updateStats() {
            document.getElementById('moves').textContent = moves;
            // --- CHANGE 7: Update pair count text in JS ---
            document.getElementById('pairs').textContent = matchedPairs + ' / 8';
            updateStars();
        }
        
        function updateStars() {
            const stars = getStars(moves);
            const starElements = document.querySelectorAll('#stars .star');
            
            starElements.forEach((star, index) => {
                if (index < stars) {
                    star.classList.add('active');
                } else {
                    star.classList.remove('active');
                }
            });
        }
        
        function getStars(moves) {
            // --- CHANGE 8: Adjust star thresholds in JS ---
            if (moves <= 12) return 3; // Was 30
            if (moves <= 18) return 2; // Was 45
            if (moves <= 25) return 1; // Was 60
            return 0;
        }
        
        function showWinMessage() {
            document.getElementById('overlay').classList.add('show');
            document.getElementById('winMessage').classList.add('show');
            document.getElementById('finalMoves').textContent = moves;
            // Prevent further card interactions while the win modal is shown
            canFlip = false;
            
            const stars = getStars(moves);
            const finalStarElements = document.querySelectorAll('#finalStars .star');
            
            finalStarElements.forEach((star, index) => {
                if (index < stars) {
                    star.classList.add('active');
                } else {
                    star.classList.remove('active');
                }
            });
        }
        
        function resetGame() {
            document.getElementById('overlay').classList.remove('show');
            document.getElementById('winMessage').classList.remove('show');
            
            flippedCards = [];
            matchedPairs = 0;
            moves = 0;
            canFlip = true;
            
            for (let i = cards.length - 1; i > 0; i--) {
                const j = Math.floor(Math.random() * (i + 1));
                [cards[i], cards[j]] = [cards[j], cards[i]];
            }
            
            initGame();
        }
        
        // Start the game
        initGame();
    </script>
</body>
</html>
"""
    
    # Inject the card data into the HTML and print the final result
    print(html.replace('__CARDS_JSON__', cards_json))

# Standard boilerplate to run the main function
if __name__ == "__main__":
    main()