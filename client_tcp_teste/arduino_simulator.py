import socket
import time


def tcp_client(host='192.168.0.121', port=5000):
    """
    Cliente TCP que se conecta a um servidor e imprime as mensagens recebidas.
    
    Args:
        host (str): Endereço IP ou nome do host do servidor. Padrão: localhost.
        port (int): Porta do servidor. Padrão: 12345.
    """
    # Cria um socket TCP/IP
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        try:
            # Conecta ao servidor
            print(f"Conectando ao servidor {host}:{port}...")
            sock.connect((host, port))
            print("Conexão estabelecida. Aguardando mensagens...")

            feedbackMessage = ""
            
            while True:
                # Recebe dados do servidor (até 1024 bytes)
                receivedMsg = sock.recv(1024).decode('utf-8')
                

                feedbackMessage += receivedMsg
        

                print(feedbackMessage)
                print(feedbackMessage.startswith("<START>") and feedbackMessage.endswith("<END>"))
                if feedbackMessage.startswith("<START>") and feedbackMessage.endswith("<END>"):
                    print(f"Mensagem recebida final: {feedbackMessage}")

                    data = feedbackMessage.replace("<START>", "").replace("<END>", "").strip()

                    index = int( data.split(",")[0])

                    if index == 0:
                        print("Índice 0 detectado. Apagando o arquivo permissions.txt...")
                        with open("permissions.txt", "w") as f:
                            f.write("") #

                    with open("permissions.txt", "a") as f:
                        f.write(f"{data}\n")
                        print(f"Mensagem de permissão {index} salva no arquivo.")

                    time.sleep(2)

                    sock.sendall("<ACK>".encode("utf-8"))
                    print("escrevendo no arquivo")
                    feedbackMessage = ""

                if not receivedMsg:
                    print("Conexão encerrada pelo servidor.")
                    break
                            
                
        except ConnectionRefusedError:
            print("Erro: Não foi possível conectar ao servidor. Verifique se o servidor está rodando.")
        except KeyboardInterrupt:
            print("\nCliente encerrado pelo usuário.")
        except Exception as e:
            print(f"Erro inesperado: {e}")

if __name__ == "__main__":
    # Configurações (altere conforme necessário)
    SERVER_HOST = '192.168.56.1'  # localhost
    SERVER_PORT = 5000        # Porta padrão
    
    # Você pode alterar o host e porta passando argumentos:
    # tcp_client('192.168.1.100', 9999)
    tcp_client(SERVER_HOST, SERVER_PORT)