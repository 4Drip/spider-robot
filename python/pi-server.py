import socket

HOST = "0.0.0.0"   # ascolta su tutte le interfacce
PORT = 5000

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen(1)

print("Server in ascolto sulla porta", PORT)

conn, addr = server.accept()
print("Connesso da:", addr)

while True:
    data = conn.recv(1024)
    
    if not data:
        break

    command = data.decode().strip()
    print("Comando ricevuto:", command)

    # Qui puoi gestire i comandi
    if command == "F":
        print("Avanti")
    elif command == "B":
        print("Indietro")
    elif command == "L":
        print("Sinistra")
    elif command == "R":
        print("Destra")

conn.close()