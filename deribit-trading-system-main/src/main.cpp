#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fmt/color.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "websocket/websocket_client.h"
#include "api/api.h"
#include "utils/utils.h"
#include "authentication/password.h"

#include "latency/tracker.h"

using namespace std;

int main() {
    // Flag to control the main loop
    bool done = false;
    char* input;
    
    // Create a websocket endpoint instance
    websocket_endpoint endpoint;

    utils::printHeader();
              
    // Main command processing loop
    while (!done) {
        // Readline provides a prompt and stores the history
        input = readline(fmt::format(fg(fmt::color::green), "deribit> ").c_str());
        if (!input) {
            break; // Exit on EOF (Ctrl+D)
        }

        string command(input);
        free(input); // Clean up the memory allocated by readline

        // Skip empty input but store valid commands in history
        if (command.empty()) {
            continue;
        }
        add_history(command.c_str());

        // Command processing
        if (command == "quit" || command == "exit") {
            done = true;
        } 
        else if (command == "help" || command == "man") {
            utils::printHelp();
        }
        else if (command.substr(0, 7) == "connect") {
            // Check if URI is provided
            if (command.length() <= 8) {
                fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, 
                           "Error: Missing URI. Usage: connect <URI>\n");
            } else {
                std::string uri = command.substr(8);
                int id = endpoint.connect(uri);
        
                if (id != -1) {
                    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, 
                               "> Successfully created connection.\n");
                    fmt::print(fg(fmt::color::cyan), "> Connection ID: {}\n", id);
                    fmt::print(fg(fmt::color::yellow), "> Status: {}\n", endpoint.get_metadata(id)->get_status());
                } else {
                    fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, 
                               "Error: Failed to create connection to {}\n", uri);
                }
            }
        }
        else if (command.substr(0, 13) == "show_messages") {
            // Show messages for a specific connection
            stringstream ss(command);
            string cmd;
            int id;

            ss >> cmd >> id;

            if (ss.fail()) {
                fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, 
                           "Error: Missing connection ID. Usage: show_messages <connection_id>\n");
            } else {
                connection_metadata::ptr metadata = endpoint.get_metadata(id);

                if (metadata) {
                    if (metadata->m_messages.empty()) {
                        fmt::print(fg(fmt::color::yellow), "> No messages for connection {}\n", id);
                    } else {
                        for (const auto& msg : metadata->m_messages) {
                            cout << msg << "\n\n";
                        }
                    }
                } else {
                    fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, 
                               "> Unknown connection id {}\n", id);
                }
            }
        }
        else if (command.substr(0, 14) == "latency_report") {
            cout << getLatencyTracker().generate_report() << endl;
        }
        else if (command.substr(0, 12) == "reset_report") {
            getLatencyTracker().reset();
        }
        else if (command.substr(0, 4) == "show") {
            // Show metadata for a specific connection
            int id = atoi(command.substr(5).c_str());
 
            connection_metadata::ptr metadata = endpoint.get_metadata(id);
            if (metadata) {
                cout << *metadata << endl;
            } else {
                cout << "> Unknown connection id " << id << endl;
            }
        }
        else if (command.substr(0, 5) == "close") {
            // Close a specific connection
            stringstream ss(command);

            string cmd;
            int id;
            int close_code = websocketpp::close::status::normal;
            string reason;

            ss >> cmd >> id >> close_code;
            getline(ss, reason);

            endpoint.close(id, close_code, reason);
        }
        else if (command.substr(0, 4) == "send") {
            // Send a message on a specific connection
            stringstream ss(command);
                
            string cmd;
            int id;
            string message = "";
            
            ss >> cmd >> id;
            getline(ss, message);
            
            endpoint.send(id, message);

            // Wait for message processing
            unique_lock<mutex> lock(endpoint.get_metadata(id)->mtx);
            endpoint.get_metadata(id)->cv.wait(lock, [&] { return endpoint.get_metadata(id)->MSG_PROCESSED; });
            endpoint.get_metadata(id)->MSG_PROCESSED = false;
        }
        else if (command == "Deribit connect") {
            // Special Deribit connection
            const std::string uri = "wss://test.deribit.com/ws/api/v2";
            int id = endpoint.connect(uri);

            if (id != -1) {
                fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, 
                           "> Successfully created connection to Deribit TESTNET.\n");
                fmt::print(fg(fmt::color::cyan), "> Connection ID: {}\n", id);
                fmt::print(fg(fmt::color::yellow), "> Status: {}\n", endpoint.get_metadata(id)->get_status());
            } else {
                fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, 
                           "> Failed to create connection to Deribit TESTNET.\n");
            }
        }
        else if (command.substr(0, 7) == "Deribit") {
            // Process Deribit-specific API commands
            int id; 
            string cmd;
        
            stringstream ss(command);
            ss >> cmd >> id;
            
            string msg = api::process(command);
            if (msg != "") {
                int success = endpoint.send(id, msg);
                if (success >= 0) {
                    unique_lock<mutex> lock(endpoint.get_metadata(id)->mtx);
                    endpoint.get_metadata(id)->cv.wait(lock, [&] { return endpoint.get_metadata(id)->MSG_PROCESSED; });
                    endpoint.get_metadata(id)->MSG_PROCESSED = false;
                }
            }
        }
        else {
            cout << "Unrecognized command" << endl;
        }
    }
    return 0;
}
