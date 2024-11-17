import serial
import time
import threading
import json
import os

CONFIG_FILE = 'config/game_config.json'


def setup_serial_port():
    """!
    @brief Sets up the serial port for communication.
    @details Prompts the user to enter the serial port (e.g., /dev/ttyUSB0 or COM3) and
             returns a serial connection object.
    @return Serial connection object.
    @throws serial.SerialException if the serial port cannot be opened.
    """
    try:
        port = input("Enter the serial port (e.g., /dev/ttyUSB0 or COM3): ")
        return serial.Serial(port, 9600, timeout=1)
    except serial.SerialException as e:
        print(f"Error: {e}")
        exit(1)


def send_message(message, ser):
    """!
    @brief Sends a message over the serial connection.
    @details Encodes the message and sends it via the given serial connection.
    @param message The message to send.
    @param ser The serial connection object.
    @throws serial.SerialException if sending the message fails.
    """
    try:
        ser.write((message + '\n').encode())
    except serial.SerialException as e:
        print(f"Error sending message: {e}")


def receive_message(ser):
    """!
    @brief Receives a message from the serial connection.
    @details Reads a line from the serial connection, decodes it, and strips it of any
             unnecessary whitespace or errors.
    @param ser The serial connection object.
    @return The received message or None if an error occurs.
    @throws serial.SerialException if receiving the message fails.
    """
    try:
        received = ser.readline().decode('utf-8', errors='ignore').strip()
        if received:
            print(received)
        return received
    except serial.SerialException as e:
        print(f"Error receiving message: {e}")
        return None


def receive_multiple_messages(ser, count):
    """!
    @brief Receives multiple messages from the serial connection.
    @details Calls the receive_message function multiple times to collect a list of received messages.
    @param ser The serial connection object.
    @param count The number of messages to receive.
    @return A list of received messages.
    """
    messages = []
    for _ in range(count):
        message = receive_message(ser)
        if message:
            messages.append(message)
    return messages


def user_input_thread(ser):
    """!
    @brief Handles user input in a separate thread.
    @details Continuously listens for user input. Depending on the input, the user can send messages
             or save/load game configurations. The thread will exit if the user types 'exit'.
    @param ser The serial connection object.
    """
    global can_input
    while True:
        if can_input:
            user_message = input()
            if user_message.lower() == 'exit':
                print("Exiting...")
                global exit_program
                exit_program = True
                break
            elif user_message.lower().startswith('save'):
                save_game_config(user_message)
            elif user_message.lower().startswith('load'):
                file_path = input("Enter the path to the configuration file: ")
                load_game_config(file_path, ser)
            send_message(user_message, ser)
            can_input = False 


def monitor_incoming_messages(ser):
    """!
    @brief Monitors incoming messages on the serial connection in a separate thread.
    @details Continuously checks for messages from the serial connection and updates the can_input
             flag when new data is received.
    @param ser The serial connection object.
    """
    global can_input
    global last_received_time
    while not exit_program:
        received = receive_message(ser)
        if received:
            last_received_time = time.time() 
            if not can_input:
                can_input = True  


def save_game_config(message):
    """!
    @brief Saves the game configuration to a JSON file.
    @details Saves game mode, player symbols, and other configurations to the `game_config.json` file.
    @param message The message containing the configuration details.
    @throws Exception if saving the configuration fails.
    """
    config = {
        "gameMode": 0,  
        "player1Symbol": 'X',
        "player2Symbol": 'O'
    }

    try:
        params = message.split()
        if len(params) == 2 and params[1] in ['0', '1', '2']:
            config["gameMode"] = int(params[1])

        with open(CONFIG_FILE, 'w') as f:
            json.dump(config, f)
        print(f"Configuration saved to {CONFIG_FILE}")
    except Exception as e:
        print(f"Error saving configuration: {e}")


def load_game_config(file_path, ser):
    """!
    @brief Loads the game configuration from a JSON file.
    @details Reads the configuration from a file and sends it to the serial device. If the file is not
             found, prompts the user to provide a valid path.
    @param file_path The path to the configuration file.
    @param ser The serial connection object.
    @throws Exception if loading the configuration fails.
    """
    try:
        if os.path.exists(file_path):
            with open(file_path, 'r') as f:
                config = json.load(f)
                game_mode = config.get("gameMode", 0)
                player1_symbol = config.get("player1Symbol", 'X')
                player2_symbol = config.get("player2Symbol", 'O')

                print(f"Game Mode: {game_mode}")
                print(f"Player 1 Symbol: {player1_symbol}")
                print(f"Player 2 Symbol: {player2_symbol}")

                json_message = {
                    "gameMode": game_mode,
                    "player1Symbol": player1_symbol,
                    "player2Symbol": player2_symbol
                }

                json_str = json.dumps(json_message)
                print(json_str)

                send_message(json_str, ser)
        else:
            print("Configuration file not found. Please provide a valid path.")
    except Exception as e:
        print(f"Error loading configuration: {e}")


if __name__ == "__main__":
    """!
    @brief Main entry point of the program.
    @details Sets up the serial port and starts two threads: one for monitoring incoming messages
             and one for handling user input. The program will keep running until the exit flag is set.
    """
    ser = setup_serial_port()
    can_input = True
    exit_program = False
    last_received_time = time.time()

    threading.Thread(target=monitor_incoming_messages, args=(ser,), daemon=True).start()
    threading.Thread(target=user_input_thread, args=(ser,), daemon=True).start()

    try:
        while not exit_program:
            if time.time() - last_received_time >= 1 and can_input:
                pass
            else:
                time.sleep(0.1)
    except KeyboardInterrupt:
        print("Exit!")
    finally:
        if ser.is_open:
            print("Closing serial port...")
            ser.close()
