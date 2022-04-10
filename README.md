# dinhof_project
## Ein einfacher Redis Client in C++

Dieser Client ermöglicht eine flexieble Kommunikation mit einem Redis Server.
Aufgrund seiner Klassenstruktur kann er beleibig erweitern und angepasst werden.


## Installation


```bash
git clone https://github.com/Marschell01/dinhof_project.git
cd dinhof_project/
meson build && cd build
ninja   
```

## Ausführung
Der Client funktioniert nur mit der bereitgestellten Proxy. Diese empfängt die Daten des Clients und sendet diese an einen Redis Server weiter.

Die Proxy wird wie folgt gestartet:
```bash
./redis_proxy --dport {Port des Redis Servers} --hport {Port, auf dem die Proxy nach einem Client lauschen soll}
```

Der Client wird wie folgt gestartet:
```bash
./redis_client --ip {IP Adresse des Proxy Servers} --port {Port des Proxy Servers}
```

## Bedienung

```cpp
/*
 * Kommunikation mit dem Server
*/
Redis::RedisClient client{ip_address, destination_port};
if (!client.is_connected()) {
    //error handling
    return 1;
}

std::string output;
output = client.execute("SET", "name", "MaxMuster123").parse<std::string>();
LOG_INFO(output);
output = client.execute("GET", "name").parse<std::string>();
LOG_INFO(output);

/*
 * Pipelining
*/
client.execute_no_flush("SET", "name", "MaxMuster321");
client.execute_no_flush("GET", "name");
std::vector<Redis::RedisResponse> responses{client.flush_pending()};

for (Redis::RedisResponse& response : responses) {
    LOG_INFO(response.parse<std::string>() << std::endl);
}

/*
 * Transaktionen
*/
output = client.execute("SET", "name", "MaxMuster321").parse<std::string>();
LOG_INFO(output);

client.begin_transaction();
output = client.execute("SET", "name", "MaxMuster123").parse<std::string>();
LOG_INFO(output);

output = client.execute("GET", "name").parse<std::string>();
LOG_INFO(output);
    
//client.end_transaction();      -> Transaktion wird durchgeführt
client.discard_transaction(); // -> Transaktion wird abgebrochen

output = client.execute("GET", "name").parse<std::string>();
LOG_INFO(output);


/*
 * Locks
*/
client.lock("resource_1");

output = client.execute("SET", "name", "MaxMuster123").parse<std::string>();
LOG_INFO(output);

output = client.execute("GET", "name").parse<std::string>();
LOG_INFO(output);

client.unlock("resource_1");


/*
 * Subscribe
*/
client.subscribe("subscribe_object");
while (true) {
    LOG_INFO("Subscriber waiting for data");
        
    std::vector<Redis::RedisResponse> responses{client.fetch_data()};
    for (Redis::RedisResponse& response : responses) {
        LOG_INFO(response.parse<std::string>());
    }
}


/*
 * Publish
*/
std::string input{};
while (true) {
    std::cout << "Element: ";
    std::getline(std::cin, input);
    output = client.execute("PUBLISH", "subscribe_object", input).parse<std::string>();
    LOG_INFO(output);
}
```
Falls ein Rückgabewert nicht in den angegebenen Datentypen geschpeichert werden kann, tritt eine Exception auf.
