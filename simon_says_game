
// ESP32 Simon Says - Simplified Web Version (No External Libraries Required)
// Uses only built-in ESP32 libraries

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#define BUZZER_CHANNEL 0


// Replace with your WiFi credentials
const char* ssid = "yous_ssisd";        // Example: "MyHomeWiFi"
const char* password = "your_pass";     // Example: "mypassword123"

const char* auth_user = "admin";
const char* auth_pass = "your_pass";

// LED GPIO Pin Configuration
int leds[] = {25, 26, 27, 14};  // Red, Green, Blue, Yellow
String ledColors[] = {"red", "green", "blue", "yellow"};

// Optional physical buttons
int buttons[] = {32, 33, 34, 35};
bool physicalButtonsEnabled = true;

// Optional buzzer
#define BUZZER_PIN 23
int tones[] = {262, 330, 392, 523}; // C, E, G, C
bool soundEnabled = true;

// Game state
int simonMemory[100];
int currentLevel = 0;
int playerIndex = 0;
bool showingSequence = false;
unsigned long sequenceStartTime = 0;
bool gameActive = false;
bool waitingForInput = false;
int highScore = 0;
int currentScore = 0;
unsigned long lastActionTime = 0;
int gameSpeed = 800;
int totalGames = 0;

// Web server
WebServer server(80);

// Game control via HTTP endpoints
String lastMessage = "Ready to play!";
String lastSequence = "";
String gameStatus = "ready";

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Simon Says - Simple Web Edition ===");
  
  setupHardware();
  connectToWiFi();
  setupWebServer();
  
  randomSeed(esp_random());
  
  Serial.println("\n✅ System ready!");
  Serial.println("📱 Open browser and navigate to:");
  Serial.println("   http://" + WiFi.localIP().toString());
}

void setupHardware() {
  // Configure LEDs
  for (int i = 0; i < 4; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }
  
  // Configure buttons
  if (physicalButtonsEnabled) {
    for (int i = 0; i < 4; i++) {
      pinMode(buttons[i], INPUT_PULLDOWN);
    }
  }
  
  // Configure buzzer 
  if (soundEnabled) {
    pinMode(BUZZER_PIN, OUTPUT);
    ledcAttach(BUZZER_PIN, 2000, 8);  // Use new v3.x function
    ledcWrite(BUZZER_PIN, 0);
  }
  
  // LED test sequence
  Serial.println("Testing LEDs...");
  for (int i = 0; i < 4; i++) {
    digitalWrite(leds[i], HIGH);
    delay(200);
    digitalWrite(leds[i], LOW);
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
    digitalWrite(leds[0], attempts % 2);
  }
  
  digitalWrite(leds[0], LOW);
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n❌ Failed to connect to WiFi!");
    return;
  }
  
  Serial.println("\n✅ WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Flash all LEDs to indicate connection
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < 4; i++) digitalWrite(leds[i], HIGH);
    delay(100);
    for (int i = 0; i < 4; i++) digitalWrite(leds[i], LOW);
    delay(100);
  }
}

void setupWebServer() {
  // Serve main page
  server.on("/", HTTP_GET, serveMainPage);
  
  // Game control endpoints
  server.on("/start", HTTP_POST, handleStartGame);
  server.on("/button", HTTP_POST, handleButtonPress);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/sequence", HTTP_GET, handleGetSequence);
  
  // Enable CORS for all endpoints
  server.enableCORS(true);
  
  server.begin();
  Serial.println("Web server started on port 80");
}

void serveMainPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Simon Says - ESP32</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            user-select: none;
            -webkit-tap-highlight-color: transparent;
        }
        
        body {
            font-family: 'Arial', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            color: white;
            padding: 20px;
        }
        
        h1 {
            font-size: 2.5em;
            margin-bottom: 20px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        
        .status {
            background: rgba(255,255,255,0.2);
            border-radius: 10px;
            padding: 15px;
            margin-bottom: 20px;
            backdrop-filter: blur(10px);
            display: flex;
            gap: 20px;
        }
        
        .stat {
            text-align: center;
        }
        
        .stat-value {
            font-size: 1.5em;
            font-weight: bold;
        }
        
        .game-board {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            width: 300px;
            height: 300px;
            margin: 20px;
            position: relative;
        }
        
        .simon-button {
            border: none;
            cursor: pointer;
            transition: all 0.3s ease;
            opacity: 0.7;
            font-size: 0;
        }
        
        .simon-button:active {
            transform: scale(0.95);
            opacity: 1;
        }
        
        .simon-button.lit {
            opacity: 1;
            transform: scale(1.05);
        }
        
        .red { 
            background: radial-gradient(circle, #ff6b6b, #c92a2a); 
            border-radius: 100% 10px 10px 10px;
        }
        .green { 
            background: radial-gradient(circle, #51cf66, #2f9e44); 
            border-radius: 10px 100% 10px 10px;
        }
        .blue { 
            background: radial-gradient(circle, #339af0, #1864ab); 
            border-radius: 10px 10px 10px 100%;
        }
        .yellow { 
            background: radial-gradient(circle, #ffd43b, #fab005); 
            border-radius: 10px 10px 100% 10px;
        }
        
        .center-circle {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            width: 80px;
            height: 80px;
            background: rgba(255,255,255,0.95);
            border-radius: 50%;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            color: #333;
            font-weight: bold;
            box-shadow: 0 5px 15px rgba(0,0,0,0.3);
        }
        
        .controls {
            display: flex;
            gap: 10px;
            margin: 20px;
        }
        
        .btn {
            background: rgba(255,255,255,0.2);
            border: 2px solid rgba(255,255,255,0.3);
            color: white;
            padding: 12px 24px;
            border-radius: 25px;
            cursor: pointer;
            font-size: 1em;
            font-weight: bold;
            transition: all 0.3s ease;
            backdrop-filter: blur(10px);
        }
        
        .btn:hover {
            background: rgba(255,255,255,0.3);
            transform: translateY(-2px);
        }
        
        .btn-primary {
            background: linear-gradient(45deg, #4CAF50, #45a049);
            border: none;
        }
        
        .btn-danger {
            background: linear-gradient(45deg, #f44336, #da190b);
            border: none;
        }
        
        .message {
            background: rgba(255,255,255,0.2);
            border-radius: 10px;
            padding: 15px;
            margin: 10px;
            text-align: center;
            backdrop-filter: blur(10px);
            min-height: 50px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.1em;
        }
        
        @media (max-width: 480px) {
            .game-board {
                width: 250px;
                height: 250px;
            }
            h1 { font-size: 2em; }
        }
    </style>
</head>
<body>
    <h1>🎮 Simon Says</h1>
    
    <div class="status">
        <div class="stat">
            <div class="stat-value" id="level">0</div>
            <div>Level</div>
        </div>
        <div class="stat">
            <div class="stat-value" id="score">0</div>
            <div>Score</div>
        </div>
        <div class="stat">
            <div class="stat-value" id="highScore">0</div>
            <div>High Score</div>
        </div>
    </div>
    
    <div class="game-board">
        <button class="simon-button red" onclick="buttonPress(0)"></button>
        <button class="simon-button green" onclick="buttonPress(1)"></button>
        <button class="simon-button blue" onclick="buttonPress(2)"></button>
        <button class="simon-button yellow" onclick="buttonPress(3)"></button>
        
        <div class="center-circle">
            <div id="centerText">👋</div>
            <div style="font-size: 0.7em;" id="centerSub">Ready</div>
        </div>
    </div>
    
    <div class="message" id="message">Press 'Start Game' to begin!</div>
    
    <div class="controls">
        <button class="btn btn-primary" onclick="startGame()">🎮 Start Game</button>
        <button class="btn btn-danger" onclick="resetGame()">🔄 Reset</button>
    </div>
    
    <script>
        let gameState = 'ready';
        let isPlayerTurn = false;
        let sequenceShowing = false;
        
        const colors = ['red', 'green', 'blue', 'yellow'];
        const frequencies = [262, 330, 392, 523];
        let audioContext;
        
        function initAudio() {
            if (!audioContext) {
                audioContext = new (window.AudioContext || window.webkitAudioContext)();
            }
        }
        
        function playTone(frequency) {
            initAudio();
            const oscillator = audioContext.createOscillator();
            const gainNode = audioContext.createGain();
            
            oscillator.connect(gainNode);
            gainNode.connect(audioContext.destination);
            
            oscillator.frequency.value = frequency;
            oscillator.type = 'sine';
            gainNode.gain.value = 0.3;
            
            oscillator.start();
            gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.3);
            
            setTimeout(() => oscillator.stop(), 300);
        }
        
        async function startGame() {
            try {
                const response = await fetch('/start', { method: 'POST' });
                if (response.ok) {
                    document.getElementById('message').textContent = 'Starting game...';
                    setTimeout(() => {
                        updateStatus();
                        pollForSequence();
                    }, 1000);
                }
            } catch (error) {
                console.error('Error starting game:', error);
            }
        }
        
        async function buttonPress(button) {
            if (!isPlayerTurn || sequenceShowing) {
                console.log('Button press ignored - not player turn or sequence showing');
                return;
            }
            
            console.log('Player pressed button:', button);
            
            const btn = document.querySelectorAll('.simon-button')[button];
            btn.classList.add('lit');
            playTone(frequencies[button]);
            
            setTimeout(() => btn.classList.remove('lit'), 300);
            
            try {
                const response = await fetch('/button', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'button=' + button
                });
                
                if (response.ok) {
                    setTimeout(() => {
                        updateStatus();
                        pollForSequence();
                    }, 300);
                }
            } catch (error) {
                console.error('Error sending button press:', error);
            }
        }
        
        async function resetGame() {
            try {
                const response = await fetch('/reset', { method: 'POST' });
                if (response.ok) {
                    isPlayerTurn = false;
                    sequenceShowing = false;
                    updateStatus();
                    document.getElementById('centerText').textContent = '👋';
                    document.getElementById('centerSub').textContent = 'Ready';
                }
            } catch (error) {
                console.error('Error resetting game:', error);
            }
        }
        
        async function updateStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                
                document.getElementById('level').textContent = data.level;
                document.getElementById('score').textContent = data.score;
                document.getElementById('highScore').textContent = data.highScore;
                document.getElementById('message').textContent = data.message;
                
                if (data.level > 0) {
                    document.getElementById('centerText').textContent = data.level;
                    document.getElementById('centerSub').textContent = 'Level';
                }
                
                gameState = data.status;
                isPlayerTurn = data.waitingForInput;
                
                console.log('Status - Level:', data.level, 'Waiting:', data.waitingForInput);
            } catch (error) {
                console.error('Error updating status:', error);
            }
        }
        
        async function pollForSequence() {
  try {
    const response = await fetch('/sequence');
    const data = await response.json();
    
    console.log('Sequence poll:', data);
    
    if (data.showing && data.sequence && data.sequence.length > 0 && !sequenceShowing) {
      console.log('Starting web sequence:', data.sequence);
      playSequence(data.sequence);
    } else if (!data.showing && data.level > 0 && sequenceShowing) {
      console.log('ESP32 sequence finished, enabling player turn');
      setTimeout(() => {
        isPlayerTurn = true;
        sequenceShowing = false;
        document.getElementById('message').textContent = 'Your turn! Repeat the sequence';
      }, 500);
    } else if (!data.showing && data.level > 0 && !sequenceShowing && !isPlayerTurn) {
      console.log('Ready for player turn');
      isPlayerTurn = true;
      document.getElementById('message').textContent = 'Your turn! Repeat the sequence';
    }
  } catch (error) {
    console.error('Error polling sequence:', error);
  }
}
        
        function playSequence(sequence) {
            isPlayerTurn = false;
            sequenceShowing = true;
            document.getElementById('message').textContent = 'Watch carefully!';
            
            let i = 0;
            const interval = setInterval(() => {
                if (i > 0) {
                    document.querySelectorAll('.simon-button')[sequence[i-1]].classList.remove('lit');
                }
                
                if (i < sequence.length) {
                    const btn = document.querySelectorAll('.simon-button')[sequence[i]];
                    btn.classList.add('lit');
                    playTone(frequencies[sequence[i]]);
                    console.log('Web flashing button:', sequence[i]);
                    i++;
                } else {
                    clearInterval(interval);
                    if (sequence.length > 0) {
                        document.querySelectorAll('.simon-button')[sequence[sequence.length-1]].classList.remove('lit');
                    }
                    setTimeout(() => {
                        sequenceShowing = false;
                        isPlayerTurn = true;
                        document.getElementById('message').textContent = 'Your turn!';
                    }, 500);
                }
            }, 900); // Match ESP32 timing
        }
        
        // Poll for sequences when game is active
        setInterval(() => {
            if (gameState === 'playing' && !isPlayerTurn && !sequenceShowing) {
                pollForSequence();
            }
        }, 500);
        
        // Regular status updates
        setInterval(updateStatus, 2000);
        
        // Initialize
        updateStatus();
</script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleStartGame() {
  Serial.println("Starting new game...");
  
  // Reset game state
  currentLevel = 0;
  playerIndex = 0;
  gameActive = true;
  currentScore = 0;
  totalGames++;
  gameStatus = "playing";
  
  // Generate new sequence
  for (int i = 0; i < 100; i++) {
    simonMemory[i] = random(0, 4);
  }
  
  lastMessage = "Game started! Watch carefully...";
  
  // Start first round
  nextRound();
  
  server.send(200, "text/plain", "Game started");
}

// handleButtonPress
void handleButtonPress() {
  if (!gameActive || !waitingForInput) {
    server.send(400, "text/plain", "Not accepting input");
    return;
  }
  
  int button = server.arg("button").toInt();
  
  if (button < 0 || button > 3) {
    server.send(400, "text/plain", "Invalid button");
    return;
  }
  
  Serial.println("Button pressed: " + String(button) + ", Expected: " + String(simonMemory[playerIndex]));
  
  // Light up the LED briefly
  digitalWrite(leds[button], HIGH);
  
  // Play sound - FIXED for v3.x
  if (soundEnabled) {
    ledcWriteTone(BUZZER_PIN, tones[button]);
  }
  
  delay(150);
  digitalWrite(leds[button], LOW);
  ledcWriteTone(BUZZER_PIN, 0);
  
  // Check if correct
  if (button == simonMemory[playerIndex]) {
    Serial.println("Correct!");
    
    playerIndex++;
    lastActionTime = millis();
    
    // Check if sequence complete
    if (playerIndex >= currentLevel) {
      Serial.println("Level complete!");
      waitingForInput = false;
      currentScore = currentLevel;
      
      if (currentScore > highScore) {
        highScore = currentScore;
        lastMessage = "NEW HIGH SCORE! Level " + String(currentLevel) + " complete!";
      } else {
        lastMessage = "Level " + String(currentLevel) + " complete!";
      }
      
      delay(1000);  // Brief pause before next level
      nextRound();
    } else {
      lastMessage = "Good! Continue the sequence...";
    }
    
    server.send(200, "text/plain", "Correct");
  } else {
    // Wrong button - game over
    Serial.println("Wrong! Game Over. You pressed: " + String(button) + ", needed: " + String(simonMemory[playerIndex]));
    gameActive = false;
    waitingForInput = false;
    showingSequence = false;  // Stop any sequence
    gameStatus = "gameover";
    
    // Quick error flash
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 4; j++) digitalWrite(leds[j], HIGH);
      delay(50);
      for (int j = 0; j < 4; j++) digitalWrite(leds[j], LOW);
      delay(50);
    }
    
    lastMessage = "Game Over! Final Score: " + String(currentScore);
    
    server.send(200, "text/plain", "Game Over");
  }
}

void handleReset() {
  Serial.println("Resetting game...");
  
  gameActive = false;
  waitingForInput = false;
  currentLevel = 0;
  playerIndex = 0;
  currentScore = 0;
  gameStatus = "ready";
  
  // Turn off all LEDs
  for (int i = 0; i < 4; i++) {
    digitalWrite(leds[i], LOW);
  }
  
  lastMessage = "Game reset! Ready to play? 🔄";
  
  server.send(200, "text/plain", "Reset");
}

void handleStatus() {
  String json = "{";
  json += "\"status\":\"" + gameStatus + "\",";
  json += "\"level\":" + String(currentLevel) + ",";
  json += "\"score\":" + String(currentScore) + ",";
  json += "\"highScore\":" + String(highScore) + ",";
  json += "\"totalGames\":" + String(totalGames) + ",";
  json += "\"waitingForInput\":" + String(waitingForInput ? "true" : "false") + ",";
  json += "\"message\":\"" + lastMessage + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleGetSequence() {
  if (gameActive && currentLevel > 0) {
    String json = "{\"sequence\":[";
    for (int i = 0; i < currentLevel; i++) {
      if (i > 0) json += ",";
      json += String(simonMemory[i]);
    }
    json += "], \"showing\": " + String(showingSequence ? "true" : "false");
    json += ", \"level\": " + String(currentLevel) + "}";
    server.send(200, "application/json", json);
    Serial.println("Sent sequence - showing: " + String(showingSequence ? "true" : "false"));
  } else {
    String json = "{\"sequence\":[], \"showing\": false, \"level\": 0}";
    server.send(200, "application/json", json);
  }
}

//NextRound function:
void nextRound() {
  currentLevel++;
  playerIndex = 0;
  waitingForInput = false;
  showingSequence = true;  // Set this BEFORE any delays
  
  Serial.println("=== Starting Level: " + String(currentLevel) + " ===");
  Serial.print("Sequence: ");
  for (int i = 0; i < currentLevel; i++) {
    Serial.print(String(simonMemory[i]) + " ");
  }
  Serial.println();
  
  lastMessage = "Level " + String(currentLevel) + " - Watch the sequence!";
  sequenceStartTime = millis();
  
  // Give web interface time to poll and see showing=true
  delay(2000); 
  
  // Play sequence on physical LEDs
  for (int i = 0; i < currentLevel; i++) {
    int led = simonMemory[i];
    
    Serial.println("Showing LED " + String(i+1) + "/" + String(currentLevel) + ": " + String(led));
    digitalWrite(leds[led], HIGH);
    
    if (soundEnabled) {
      ledcWriteTone(BUZZER_PIN, tones[led]);
    }
    
    delay(700);
    
    digitalWrite(leds[led], LOW);
    ledcWriteTone(BUZZER_PIN, 0);
    
    if (i < currentLevel - 1) {
      delay(200);
    }
  }
  
  // Keep showing=true for additional time to ensure web sees it
  delay(1000);
  
  showingSequence = false;  // NOW set to false
  waitingForInput = true;
  lastActionTime = millis();
  lastMessage = "Your turn! Repeat the sequence";
  
  Serial.println("Sequence complete. Waiting for player input...");
}

void checkPhysicalButtons() {
  if (!physicalButtonsEnabled || !gameActive || !waitingForInput) return;
  
  static bool lastButtonStates[4] = {false, false, false, false};
  static unsigned long lastDebounceTime[4] = {0, 0, 0, 0};
  const unsigned long debounceDelay = 50;
  
  for (int i = 0; i < 4; i++) {
    bool currentState = digitalRead(buttons[i]);
    
    if (currentState != lastButtonStates[i]) {
      lastDebounceTime[i] = millis();
    }
    
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (currentState && !lastButtonStates[i]) {
        Serial.println("Physical button pressed: " + String(i) + ", Expected: " + String(simonMemory[playerIndex]));
        
        // Light up the LED
        digitalWrite(leds[i], HIGH);
        
        // Play sound - FIXED for v3.x
        if (soundEnabled) {
          ledcWriteTone(BUZZER_PIN, tones[i]);
        }
        
        delay(150);
        digitalWrite(leds[i], LOW);
        ledcWriteTone(BUZZER_PIN, 0);
        
        // Same game logic as web button
        if (i == simonMemory[playerIndex]) {
          Serial.println("Correct!");
          playerIndex++;
          lastActionTime = millis();
          
          if (playerIndex >= currentLevel) {
            Serial.println("Level complete!");
            waitingForInput = false;
            currentScore = currentLevel;
            
            if (currentScore > highScore) {
              highScore = currentScore;
              lastMessage = "NEW HIGH SCORE! Level " + String(currentLevel) + " complete!";
            } else {
              lastMessage = "Level " + String(currentLevel) + " complete!";
            }
            
            delay(1000);
            nextRound();
          } else {
            lastMessage = "Good! Continue the sequence...";
          }
        } else {
          // Wrong button - game over
          Serial.println("Wrong! Game Over. You pressed: " + String(i) + ", needed: " + String(simonMemory[playerIndex]));
          gameActive = false;
          waitingForInput = false;
          showingSequence = false;
          gameStatus = "gameover";
          
          // Quick error flash
          for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 4; k++) digitalWrite(leds[k], HIGH);
            delay(50);
            for (int k = 0; k < 4; k++) digitalWrite(leds[k], LOW);
            delay(50);
          }
          
          lastMessage = "Game Over! Final Score: " + String(currentScore);
        }
        
        break; // Only handle one button press at a time
      }
    }
    
    lastButtonStates[i] = currentState;
  }
}

void loop() {
  server.handleClient();
  
  // Check physical buttons
  if (physicalButtonsEnabled) {
    checkPhysicalButtons();
  }
  
  // Check timeout (10 seconds)
  if (gameActive && waitingForInput && (millis() - lastActionTime > 10000)) {
    Serial.println("Player timeout!");
    gameActive = false;
    waitingForInput = false;
    gameStatus = "timeout";
    lastMessage = "Timeout! Game over ⏰";
    
    // Flash LEDs for timeout
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 4; j++) digitalWrite(leds[j], HIGH);
      delay(200);
      for (int j = 0; j < 4; j++) digitalWrite(leds[j], LOW);
      delay(200);
    }
  }
  
  delay(10);
}
