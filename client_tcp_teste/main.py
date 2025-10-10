import socket
import threading
import time
import json
import itertools
from datetime import datetime, timedelta

HOST = "192.168.0.121"
PORT = 5000



def generate_schedules():
    schedules = []
    # LISTA DE IDs VÁLIDOS (6 dígitos, como solicitado)
    # Use IDs que correspondem aos usuários reais
    pin_ids_validos = [
        "100001", 
        "200002", 
        "300003", 
        "400004",
        "500005", 
        "600006", 
        "700007", 
        "800008",
        "900009", 
        "010101", 
        "020202", 
        "030303"
    ]
    
    # Usamos o itertools.cycle para garantir que teremos um ID para cada agendamento
    # Se o número de agendamentos for maior que o número de IDs, a lista de IDs se repete.
    id_iterator = itertools.cycle(pin_ids_validos)
    
    # Configurações fixas para o agendamento
    day = 3
    month = 10
    year = 25

    # Gera agendamentos a cada 2 horas (0h, 2h, 4h...22h) - 12 iterações
    for start_hour in range(0, 24, 2):
        
        # Pega o próximo ID da lista
        current_pin_id = next(id_iterator)
        
        end_hour = start_hour + 2
        
        # Ajusta para o caso da última janela (22h-0h)
        if end_hour >= 24:
            end_hour = 0
        
        schedule = {
            # O PIN/ID de 6 dígitos agora é o primeiro campo
            'pincode': current_pin_id, 
            'startHour': 10,
            'startMinute': 0,
            'endHour': 13,
            'endMinute': 0,
            'day': day,
            'month': month,
            'year': year
        }
        schedules.append(schedule)

    return schedules

def send_schedule_to_client(conn):
    while True:
        time.sleep(0.1)
        all_schedules = generate_schedules()
    
        print(all_schedules)

        conn.sendall(b"<CLEAN>")

        time.sleep(1)

        for schedule in all_schedules:
            data_str = (
                f"{schedule['pincode']},"
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

          
            try:
                ack = conn.recv(1024)
                if ack.decode('utf-8') == "<ACK>":
                    print("enviando proximo")
                else:
                    break
            except socket.timeout:
                break
            except Exception as e:
                break    
            
            time.sleep(0.1)
    
        time.sleep(60)

def handle_client(conn, addr):
    print(f"Cliente conectado: {addr}")
    try:        
        update_thread = threading.Thread(target=send_schedule_to_client, args=
                                         (conn,), daemon=True)
        update_thread.start()
        
        while True:
            pass
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