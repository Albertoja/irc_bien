#include "Server.hpp"

bool RunServer = true;
ChannelData *todos();


void UseCtrl(int signal)
{
    signal = 0;
    RunServer = false;
}

Server::Server(std::string const port, std::string const pass) : _port(port), _pass(pass){_supass = "supermario";}
Server::Server(const Server &other){*this = other;}
Server::~Server(){}
Server &Server::operator=(const Server &other)
{
    if (this != &other)
    {
        _port = other._port;
        _pass = other._pass;
    }
    return(*this);
}

int Server::contLines( std::string& cadena) 
{
    int contador = 0;
    for (size_t i = 0; i < cadena.length(); ++i) {
        if (cadena[i] == '\n') {
            contador++;
        }
    }
    return contador;
}

int Server::CreateNewUser(struct sockaddr_storage client_addr, int server_socket)
{
    int client_socket;
    socklen_t client_len;

    client_len = sizeof(client_addr);
    client_socket = accept(server_socket,(struct sockaddr *) &client_addr, &client_len);
    if(client_socket == -1)
            std::cerr << RED << "Error client socket" << NOCOLOR << std::endl;
    _sockets.push_back(pollfd());
    _sockets.back().fd = client_socket;
    _sockets.back().events = POLLIN;
    ClientData* client = new ClientData(client_socket);
    client->setConnectionTime(time(NULL));
    client->setSocketNum(_sockets.size() - 1);
    clients_vec_login.push_back(client);
    std::cout << RED << "New user try to connect" << NOCOLOR << std::endl;
    return 0;
}
int Server::ReceiveDataClient_login(size_t socket_num, std::string line)
{
    ClientData *it_client = find_ClientData_Socket_login(_sockets[socket_num].fd);
    if(it_client == NULL)
        return 1;
    it_client->setOldMsg(it_client->getOldMsg() + line);
    if (!it_client->getOldMsg().empty() && it_client->getOldMsg()[it_client->getOldMsg().size() - 1] == '\n') 
    {
        std::string lines_str = it_client->getOldMsg();
        int lines_n = contLines(lines_str);
        if(lines_n > 0)
            lines_n -= 1;
        lines = splitString(lines_str, "\n");
        int a = 0;
        while(lines_n >= a)
        {
            if(firstCommand(it_client, lines[a]) == 0)
            {
                    if(it_client->getfirstLogin() == true && it_client->getAll() == true)
                    {
                        
                        it_client->setfirstLogin(false);
                        std::cout << GREEN << "New user connected" << NOCOLOR << std::endl;
                        clients_vec.push_back(it_client);
                        for (std::vector<ClientData*>::iterator it = clients_vec_login.begin(); it != clients_vec_login.end(); ++it)
                        {
                            if ((*it) == it_client)
                            {
                                clients_vec_login.erase(it);
                                break;
                            }
                        }
                        sendWelcomeMessageToUser(it_client);
                        lines.clear();
                        return(0);
                    }
            }
            a++;
        }
    }
    lines.clear();
    return 0;
}

int Server::ReceiveDataClient(size_t socket_num, std::string line, int bytes)
{
    if (line.empty())
        return(0);
    ClientData *it_client = find_ClientData_Socket(_sockets[socket_num].fd);
    if(it_client == NULL)
    {
        if(ReceiveDataClient_login(socket_num, line) != 0)
        {
            std::cerr << "ERROR not in socket list" << std::endl;
            args.clear();
            return(1);
        }
    }
    else if(bytes <= 0)
    {
        deleteClient(socket_num, find_ClientData_Socket(socket_num));
        args.clear();
        return(1);
    }
    else
    {
        it_client->setOldMsg(it_client->getOldMsg() + line);
        if (!it_client->getOldMsg().empty() && it_client->getOldMsg()[it_client->getOldMsg().size() - 1] == '\n') 
        {
            std::string lines_str = it_client->getOldMsg();
            int lines_n = contLines(lines_str);
            if(lines_n > 0)
                lines_n -= 1;
            lines = splitString(lines_str, "\n");
            int a = 0;
            while(lines_n >= a)
            {
                if(processCommand(it_client, socket_num, lines[a]) != 0)
                {
                    args.clear();
                    lines.clear();
                    return(1);
                }
                a++;
            }
        }
    }
    args.clear();
    lines.clear();
    return(0);
}


void Server::createChanels()
{
    
    ChannelData *all = new ChannelData("#all", "#all A public channel where everyone is welcome to engage in discussions on various topics.", "passall");
    channel_vec.push_back(all);
    ChannelData *news = new ChannelData("#news", "#news Stay updated with current news and events.", "passnews");
    channel_vec.push_back(news);
    ChannelData *cars = new ChannelData("#cars", "#cars Dive into discussions about automobiles and everything related to cars.", "passcars");
    channel_vec.push_back(cars);
    ChannelData *music = new ChannelData("#music", "#music Explore the latest music releases and share your favorite tunes.", "passmusic");
    channel_vec.push_back(music);
    ChannelData *games = new ChannelData("#games", "#games Join the gaming community to discuss video games and the latest releases.", "passgames");
    channel_vec.push_back(games);
    ChannelData *movies= new ChannelData("#movies", "#movies Delve into conversations about classic films and cinematic masterpieces.", "passmovies");
    channel_vec.push_back(movies);

}




int Server::Start()
{
    int i;
    std::string input;
    int server_socket;
    struct sockaddr_storage client_addr;
    server_socket = create_serversocket();
    _sockets.push_back(pollfd());
    _sockets[0].fd = server_socket;
    _sockets[0].events = POLLIN;
    std::cout << "IRC server listening on port " << _port << std::endl;
    signal(SIGINT, UseCtrl);
    createChanels();
    while (RunServer)
    {
        if(poll(&_sockets[0], _sockets.size(), 0) == -1 && RunServer)
        {
            std::cerr << RED << "Error poll" << NOCOLOR << std::endl;
            CloseServer();
            exit(0);
        }
        for(size_t socket_num = 0; socket_num < _sockets.size(); socket_num++)
        {
            char buffer[BUFFER_SIZE];
            bzero(buffer, BUFFER_SIZE);
            
            if(_sockets[socket_num].revents & POLLIN)
            {
                if(_sockets[socket_num].fd == server_socket)
                {
                    if(CreateNewUser(client_addr, server_socket) != 0)
                    {
                        break;
                    }
                }
                else
                {
                    int bytes;
                    bytes = recv(_sockets[socket_num].fd , buffer, BUFFER_SIZE, 0);
                    ClientData *it_client;
                    it_client = find_ClientData_Socket(_sockets[socket_num].fd);
                    if(!it_client)
                        it_client = find_ClientData_Socket_login(_sockets[socket_num].fd);
                    if(bytes <= 0)
                    { 
                        deleteClient(socket_num, it_client);
                        break;
                    }
                    else if(bytes == 1)
                    {
                        break;
                    }
                    buffer[bytes] = '\0';
                    std::string str;
                    str.assign(buffer, 0, bytes);
                    int lines_n = contLines(str);
                    if(lines_n > 0)
                        lines_n -= 1;
                    lines = splitString(str, "\n");
                    i = ReceiveDataClient(socket_num, (str), bytes);
                    if(i == 2)
                    {
                        lines.clear();
                        return(0);
                    }
                    lines.clear();
                    if(i == 1)
                        break;
                }

            }
        if(RunServer == false)
        {
            CloseServer();
            exit(0);
        }
        }
        if(!clients_vec_login.empty())
        {
            for (std::vector<ClientData*>::iterator it = clients_vec_login.begin(); it != clients_vec_login.end(); ++it)
            {
                time_t currentTime = time(NULL);
                if (currentTime - (*it)->getConnectionTime() > TIMEOUT)
                {
                    std::cout << "Timeout. Closing connection." << std::endl;
                    deleteClient((*it)->getSocketNum(), (*it));
                    break;
                }
            }
        }
    }
    CloseServer();
    exit(0);
    return(0);
}


