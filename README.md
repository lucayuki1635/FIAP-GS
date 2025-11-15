# FIAP-GS

redes_seguras = Lista de redes Wi-Fi consideradas seguras.

registrar_alerta = Exibe no console um alerta indicando que uma rede não autorizada foi detectada.

eh_rede_segura = Verifica se uma rede Wi-Fi está presente na lista de redes seguras.

tarefa_scan = Monitora periodicamente a rede Wi-Fi acessada, identificando se é segura ou não, e envia alertas para a fila quando necessário.

tarefa_alerta = Recebe SSIDs da fila e chama a função registrar_alerta para registrar/logar as redes não seguras.

tarefa_wdt_monitor = Monitora a memória livre do sistema e verifica o estado das tarefas; reinicia tarefas travadas para manter o sistema funcionando.

semaforo_redes = Mutex que protege o acesso à lista de redes seguras, evitando condições de corrida quando múltiplas tarefas a consultam simultaneamente.


<img width="1137" height="681" alt="Output" src="https://github.com/user-attachments/assets/73d1ff97-c3a0-49ad-9ad8-786ed6365b0f" />
