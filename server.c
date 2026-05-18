#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/select.h>
#include <fstream>

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

struct Patient
{
    string name;
    int age;
    double height;
    double weight;
    string blood_type;
    double temperature;
    int heart_rate;
    int respiratory_rate;
    double oxygen_level;
    string blood_pressure;
};

vector<Patient> patients =
{
    {"John Doe", 30, 180.5, 75.0, "A+", 36.7, 72, 16, 98.0, "120/80"},
    {"Jane Smith", 28, 165.0, 60.0, "B-", 37.1, 80, 18, 97.5, "118/78"},
    {"Alice Brown", 40, 170.0, 68.0, "O+", 36.5, 76, 20, 96.8, "119/79"}
};

void randomizePatientData()
{
    srand(time(0));
    for (auto &patient : patients)
    {
        patient.temperature = 30.0 + static_cast<double>(rand() % 100) / 10.0; // 30.0 to 40.0
        patient.heart_rate = 60 + rand() % 61; // 60 to 120
        patient.respiratory_rate = 12 + rand() % 9; // 12 to 20
        patient.oxygen_level = 90.0 + static_cast<double>(rand() % 11); // 90.0 to 100.0
        int systolic = 110 + rand() % 21; // 110 to 130
        int diastolic = 70 + rand() % 11; // 70 to 80
        patient.blood_pressure = to_string(systolic) + "/" + to_string(diastolic);
    }
}

void logWarningsToFile(const string& warnings)
{
    ofstream file("warnings.txt", ios::app);
    if (file.is_open())
    {
        file << warnings << endl;
        file.close();
    }
    else
    {
        cout << "Unable to open warnings file!" << endl;
    }
}

void clearWarningsFile()
{
    ofstream file("warnings.txt", ios::trunc);
    if (file.is_open())
    {
        file.close();
    }
    else
    {
        cout << "Unable to clear warnings file!" << endl;
    }
}

int main()
{
    int server_fd, client_socket, max_sd, activity, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    fd_set readfds;

    vector<int> client_sockets(MAX_CLIENTS, 0);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    cout << "Server listening on port " << PORT << endl;

    while (true)
    {
        FD_ZERO(&readfds);

        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = client_sockets[i];

            if (sd > 0)
            {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd)
            {
                max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            perror("Select error");
        }

        if (FD_ISSET(server_fd, &readfds))
        {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            cout << "New client connected." << endl;

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds))
            {
                memset(buffer, 0, BUFFER_SIZE);
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0)
                {
                    cout << "Client disconnected." << endl;
                    close(sd);
                    client_sockets[i] = 0;
                }
                else
                {
                    string command(buffer);
                    string response;

                    if (command == "random")
                    {
                        randomizePatientData();
                        response = "Randomized patient data.";
                    }
                    else if (command.rfind("info :", 0) == 0)
                    {
                        string name = command.substr(7); // Extract patient name
                        bool found = false;
                        for (const auto &patient : patients)
                        {
                            if (patient.name == name)
                            {
                                response = "Name: " + patient.name + "\n" +
                                           "Age: " + to_string(patient.age) + "\n" +
                                           "Height: " + to_string(patient.height) + "\n" +
                                           "Weight: " + to_string(patient.weight) + "\n" +
                                           "Blood Type: " + patient.blood_type + "\n" +
                                           "Temperature: " + to_string(patient.temperature) + "\n" +
                                           "Heart Rate: " + to_string(patient.heart_rate) + "\n" +
                                           "Respiratory Rate: " + to_string(patient.respiratory_rate) + "\n" +
                                           "Oxygen Level: " + to_string(patient.oxygen_level) + "%\n" +
                                           "Blood Pressure: " + patient.blood_pressure;
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            response = "Patient not found.";
                        }
                    }
                    else if (command == "clear")
                    {
                        clearWarningsFile();
                        response = "Warnings cleared.";
                    }
                    else if (command == "temperature")
                    {
                        response = "Temperatures of all patients:\n";
                        for (const auto &patient : patients)
                        {
                            response += "Name: " + patient.name + "; Temperature: " + to_string(patient.temperature) + "\n";
                        }
                    }
                    else if (command == "heart_rate")
                    {
                        response = "Heart rates of all patients:\n";
                        for (const auto &patient : patients)
                        {
                            response += "Name: " + patient.name + "; Heart Rate: " + to_string(patient.heart_rate) + "\n";
                        }
                    }
                    else if (command == "respiratory_rate")
                    {
                        response = "Respiratory rates of all patients:\n";
                        for (const auto &patient : patients)
                        {
                            response += "Name: " + patient.name + "; Respiratory Rate: " + to_string(patient.respiratory_rate) + "\n";
                        }
                    }
                    else if (command == "oxygen_level")
                    {
                        response = "Oxygen levels of all patients:\n";
                        for (const auto &patient : patients)
                        {
                            response += "Name: " + patient.name + "; Oxygen Level: " + to_string(patient.oxygen_level) + "%\n";
                        }
                    }
                    else if (command == "blood_pressure")
                    {
                        response = "Blood pressures of all patients:\n";
                        for (const auto &patient : patients)
                        {
                            response += "Name: " + patient.name + "; Blood Pressure: " + patient.blood_pressure + "\n";
                        }
                    }
                    else if (command == "age")
                    {
                        response = "Ages of all patients:\n";
                        for (const auto &patient : patients)
                        {
                            response += "Name: " + patient.name + "; Age: " + to_string(patient.age) + "\n";
                        }
                    }
                    else if (command == "height")
                    {
                        response = "Heights of all patients:\n";
                        for (const auto &patient : patients)
                        {
                            response += "Name: " + patient.name + "; Height: " + to_string(patient.height) + " cm\n";
                        }
                    }
                    else if (command == "weight")
                    {
                        response = "Weights of all patients:\n";
                        for (const auto &patient : patients)
                        {
                            response += "Name: " + patient.name + "; Weight: " + to_string(patient.weight) + " kg\n";
                        }
                    }
                    else if (command == "blood_type")
                    {
                        response = "Blood types of all patients:\n";
                        for (const auto &patient : patients)
                        {
                            response += "Name: " + patient.name + "; Blood Type: " + patient.blood_type + "\n";
                        }
                    }
                    else
                    {
                        response = "Unknown command.";
                    }

                    if (command == "random")
                    {
                        string warnings = "Warnings:\n";
                        bool warning_found = false;

                        for (const auto &patient : patients)
                        {
                            if (patient.temperature < 35.0 || patient.temperature > 38.0)
                            {
                                warnings += "Name: " + patient.name + " - Abnormal Temperature: " + to_string(patient.temperature) + "\n";
                                warning_found = true;
                            }
                            if (patient.heart_rate < 60 || patient.heart_rate > 100)
                            {
                                warnings += "Name: " + patient.name + " - Abnormal Heart Rate: " + to_string(patient.heart_rate) + "\n";
                                warning_found = true;
                            }
                            if (patient.respiratory_rate < 12 || patient.respiratory_rate > 20)
                            {
                                warnings += "Name: " + patient.name + " - Abnormal Respiratory Rate: " + to_string(patient.respiratory_rate) + "\n";
                                warning_found = true;
                            }
                            if (patient.oxygen_level < 95.0 || patient.oxygen_level > 100.0)
                            {
                                warnings += "Name: " + patient.name + " - Abnormal Oxygen Level: " + to_string(patient.oxygen_level) + "%\n";
                                warning_found = true;
                            }
                            int systolic, diastolic;
                            sscanf(patient.blood_pressure.c_str(), "%d/%d", &systolic, &diastolic);
                            if (systolic < 90 || systolic > 140 || diastolic < 60 || diastolic > 90)
                            {
                                warnings += "Name: " + patient.name + " - Abnormal Blood Pressure: " + patient.blood_pressure + "\n";
                                warning_found = true;
                            }
                        }

                        if (warning_found)
                        {
                            logWarningsToFile(warnings);
                            for (int client_sd : client_sockets)
                            {
                                if (client_sd > 0)
                                {
                                    send(client_sd, warnings.c_str(), warnings.length(), 0);
                                }
                            }
                        }
                    }

                    send(sd, response.c_str(), response.length(), 0);
                }
            }
        }
    }

    return 0;
}
