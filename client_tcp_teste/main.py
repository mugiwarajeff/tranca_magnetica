import socket
import threading
import time
import json
from datetime import datetime, timedelta

HOST = "192.168.0.103"
PORT = 5000

def generate_schedules():
    index = 0
    schedules = []
    
    # Configurações fixas
    user_id = "jeffersonc"
    password = "1234"
    day = 26
    month = 11
    year = 24

# Gera agendamentos a cada 2 horas (0h, 2h, 4h...22h)
    for start_hour in range(0, 24, 2):
        end_hour = start_hour + 2
        
        # Ajusta para o caso da última janela (22h-0h)
        if end_hour >= 24:
            end_hour = 0
        
        schedule = {
            'index': index,
            'userId': user_id,
            'password': password,
            'startHour': start_hour,
            'startMinute': 0,
            'endHour': end_hour,
            'endMinute': 0,
            'day': day,
            'month': month,
            'year': year
        }
        schedules.append(schedule)
        index += 1

    return schedules

def send_schedule_to_client(conn):
    while True:
        time.sleep(0.1)
        all_schedules = generate_schedules()
    
        print(all_schedules)

        for schedule in all_schedules:
            data_str = (
                f"{schedule['index']},"
                f"{schedule['userId']},"
                f"{schedule['password']},"
                f"{schedule['startHour']},"
                f"{schedule['startMinute']},"
                f"{schedule['endHour']},"
                f"{schedule['endMinute']},"
                f"{schedule['day']},"
                f"{schedule['month']},"
                f"{schedule['year']}"
            )

            conn.sendall(b"<START>")
            conn.sendall(data_str.encode('utf-8'))
            conn.sendall(b"<END>")
            time.sleep(0.1)
    
        time.sleep(60)

def handle_client(conn, addr):
    print(f"Cliente conectado: {addr}")
    try:
        conn.sendall(b"ACK")
        
        update_thread = threading.Thread(target=send_schedule_to_client, args=
                                         (conn,), daemon=True)
        update_thread.start()
        
        while True:
            data = conn.recv(1024)
            if not data:
                break
            print(f"[{addr}] Dados recebidos: {data.decode()}")
    except Exception as e:
        print(f"Erro com {addr}: {e}")
    finally:
        conn.close()
        print(f"Conexão encerrada: {addr}")

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        print(f"Servidor TCP aguardando conexões na porta {PORT}...")

        while True:
            conn, addr = s.accept()

            client_thread = threading.Thread(target=handle_client, args=(conn, addr))
            client_thread.start()

if __name__ == "__main__":
    main()            