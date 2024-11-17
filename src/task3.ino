#include <Arduino.h>
#include <ArduinoJson.h>

char board[3][3];
bool gameActive = false;
String player1Symbol = "X";
String player2Symbol = "O";
String currentPlayer = "X";
int gameMode = 0;

struct GameConfig {
  int gameMode;
  String player1Symbol;
  String player2Symbol;
  String currentPlayer;
};


void saveConfig(const GameConfig &config) {
  StaticJsonDocument<200> doc;
  doc["gameMode"] = config.gameMode;
  doc["player1Symbol"] = config.player1Symbol; 
  doc["player2Symbol"] = config.player2Symbol; 
  doc["currentPlayer"] = config.currentPlayer;

  String output;
  serializeJson(doc, output);
  Serial.println(output);
}

void loadConfig(String jsonConfig) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonConfig);
  
  if (error) {
    Serial.println("Failed to load configuration");
    return;
  }

  if (doc.containsKey("gameMode")) {
    gameMode = doc["gameMode"].as<int>();
  } else {
    Serial.println("gameMode not found");
    return;
  }

  if (doc.containsKey("player1Symbol") && doc["player1Symbol"].is<String>()) {
    player1Symbol = doc["player1Symbol"].as<String>(); 
  } else {
    Serial.println("player1Symbol not found or invalid");
    return;
  }

  if (doc.containsKey("player2Symbol") && doc["player2Symbol"].is<String>()) {
    player2Symbol = doc["player2Symbol"].as<String>();
  } else {
    Serial.println("player2Symbol not found or invalid");
    return;
  }

  Serial.println("Configuration loaded!");
}


void initializeBoard() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      board[i][j] = ' ';
    }
  }
}

void printBoard() {
  String boardState = "Board state:\n";
  
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (board[i][j] == 'X' || board[i][j] == 'O') {
        boardState += board[i][j]; 
      } else {
        boardState += '.';
      }
      if (j < 2) boardState += "|"; 
    }
    if (i < 2) boardState += "\n-+-+-\n"; 
    else boardState += "\n"; 
  }

  Serial.println(boardState);
}

bool checkWin(char player) {
  for (int i = 0; i < 3; i++) {
    if ((board[i][0] == player && board[i][1] == player && board[i][2] == player) || 
        (board[0][i] == player && board[1][i] == player && board[2][i] == player)) { 
      return true;
    }
  }
  
  if ((board[0][0] == player && board[1][1] == player && board[2][2] == player) ||
      (board[0][2] == player && board[1][1] == player && board[2][0] == player)) {
    return true;
  }
  
  return false;
}

bool isBoardFull() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (board[i][j] == ' ') {
        return false;
      }
    }
  }
  return true; 
}

void aiMove(char aiSymbol) {
  if (blockOpponentMove(aiSymbol == 'X' ? 'O' : 'X')) {
    return; 
  }

  int startX = random(3); 
  int startY = random(3);

  if (random(2) == 0) {
    startX = 0; 
    startY = 0;
  }

  for (int i = startX; i < 3; i++) {
    for (int j = startY; j < 3; j++) {
      if (board[i][j] == ' ') {
        board[i][j] = aiSymbol;
        Serial.println("AI played at: " + String(i + 1) + " " + String(j + 1));
        return;
      }
    }
  }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (board[i][j] == ' ') {
        board[i][j] = aiSymbol;
        Serial.println("AI played randomly at: " + String(i + 1) + " " + String(j + 1));
        return;
      }
    }
  }
}

bool blockOpponentMove(char opponent) {
  for (int i = 0; i < 3; i++) {
    // Горизонтальні та вертикальні лінії
    if (canBlock(i, 0, i, 1, i, 2, opponent)) {
      return true;
    }
    if (canBlock(0, i, 1, i, 2, i, opponent)) {
      return true;
    }
  }
  
  if (canBlock(0, 0, 1, 1, 2, 2, opponent)) {
    return true;
  }
  if (canBlock(0, 2, 1, 1, 2, 0, opponent)) {
    return true;
  }

  return false;
}

bool canBlock(int x1, int y1, int x2, int y2, int x3, int y3, char opponent) {
  if (board[x1][y1] == opponent && board[x2][y2] == opponent && board[x3][y3] == ' ') {
    board[x3][y3] = 'O'; 
    Serial.println("AI blocked opponent's winning move at: " + String(x3 + 1) + " " + String(y3 + 1));
    return true;
  }
  if (board[x1][y1] == opponent && board[x2][y2] == ' ' && board[x3][y3] == opponent) {
    board[x2][y2] = 'O'; 
    Serial.println("AI blocked opponent's winning move at: " + String(x2 + 1) + " " + String(y2 + 1));
    return true;
  }
  if (board[x1][y1] == ' ' && board[x2][y2] == opponent && board[x3][y3] == opponent) {
    board[x1][y1] = 'O'; 
    Serial.println("AI blocked opponent's winning move at: " + String(x1 + 1) + " " + String(y1 + 1));
    return true;
  }
  
  return false;
}

void processMove(String input) {
  int row = input[0] - '1';
  int col = input[2] - '1';

  if (row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == ' ') {
    if (gameMode == 1) { 
      board[row][col] = (currentPlayer == "X") ? 'X' : 'O';
    } else { 
      board[row][col] = 'X'; 
    }
    printBoard();

    if (checkWin('X')) {
      Serial.println("Player X wins!");
      gameActive = false;
      return;
    }

    if (checkWin('O')) {
      Serial.println("Player O wins!");
      gameActive = false;
      return;
    }

    if (isBoardFull()) {
      Serial.println("It's a draw!");
      gameActive = false;
      return;
    }

    if (gameMode == 2) {
      aiMove(player1Symbol[0]);
      if (checkWin(player1Symbol[0])) {
        Serial.println("Player 1 (AI) wins!");
        gameActive = false;
        return;
      }
      aiMove(player2Symbol[0]);
      if (checkWin(player2Symbol[0])) {
        Serial.println("Player 2 (AI) wins!");
        gameActive = false;
        return;
      }
    }

    currentPlayer = (currentPlayer == "X") ? 'O' : 'X';
  } else {
    Serial.println("Invalid move, try again.");
  }
}


void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    String receivedMessage = Serial.readStringUntil('\n');
    receivedMessage.trim(); 

    if (receivedMessage == "new") {
      initializeBoard();
      gameActive = true;
      
      if (gameMode == 1) {
        Serial.println("Player 1, choose your symbol: X or O");
        currentPlayer = (random(2) == 0) ? 'X' : 'O'; 
        player1Symbol = currentPlayer;
        player2Symbol = (currentPlayer == "X") ? 'O' : 'X';
        Serial.println("Player 1 is " + String(player1Symbol));
        Serial.println("Player 2 is " + String(player2Symbol));
      } else {
        currentPlayer = 'X'; 
      }

      Serial.println("New game started! " + String(currentPlayer) + " goes first.");
      printBoard();

      if (gameMode == 0) {
        while (gameActive) {
          if (currentPlayer == "X") {
            Serial.println("Your move, player (enter row and column):");
            while (Serial.available() == 0) {
            }
            String userMove = Serial.readStringUntil('\n');
            processMove(userMove); 
            printBoard();
            
            if (checkWin('X')) {
              Serial.println("Player X wins!");
              gameActive = false;
              break;
            }
            if (isBoardFull()) {
              Serial.println("It's a draw!");
              gameActive = false;
              break;
            }

            currentPlayer = 'O'; 
          } else {
            aiMove('O');
            printBoard();
            if (checkWin('O')) {
              Serial.println("AI O wins!");
              gameActive = false;
              break;
            }
            if (isBoardFull()) {
              Serial.println("It's a draw!");
              gameActive = false;
              break;
            }

            currentPlayer = 'X'; 
          }
        }
      }
      else if (gameMode == 2) {
        while (gameActive) {
          aiMove('X');  
          printBoard();
          if (checkWin('X')) {
            Serial.println("AI X wins!");
            gameActive = false;
            break; 
          }
          if (isBoardFull()) {
            Serial.println("It's a draw!");
            gameActive = false;
            break; 
          }

          aiMove('O');  
          printBoard();
          if (checkWin('O')) {
            Serial.println("AI O wins!");
            gameActive = false;
            break; 
          }
          if (isBoardFull()) {
            Serial.println("It's a draw!");
            gameActive = false;
            break; 
          }
        }
      }
    } else if (receivedMessage.startsWith("save")) {
      GameConfig config = { gameMode, player1Symbol, player2Symbol, currentPlayer };
      saveConfig(config);
    } else if (receivedMessage.startsWith("{")) {
      if (receivedMessage.length() > 0) {
          loadConfig(receivedMessage);
      } else {
          Serial.println("No message received");
      }
    } else if (receivedMessage.startsWith("modes")) {
      if (receivedMessage == "modes 0") {
        gameMode = 0;
        Serial.println("Game mode: Man vs AI");
      } else if (receivedMessage == "modes 1") {
        gameMode = 1;
        Serial.println("Game mode: Man vs Man");
      } else if (receivedMessage == "modes 2") {
        gameMode = 2;
        Serial.println("Game mode: AI vs AI");
      }
    } else if (gameActive) {
      processMove(receivedMessage);
    } else {
      Serial.println("No active game. Type 'new' to start.");
    }
  }
}
