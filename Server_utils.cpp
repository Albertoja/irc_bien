#include "Server.hpp"

ClientData *Server::find_ClientData_Nickname(std::string str)
{
    for (std::vector<ClientData*>::iterator it = clients_vec.begin(); it != clients_vec.end(); ++it)
    {
        if ((*it)->getNickName() == str)
            return (*it);
    }
    return NULL;
}

ClientData	*Server::find_ClientData_Socket(int fd)
{
    
    for (std::vector<ClientData*>::iterator it = clients_vec.begin(); it != clients_vec.end(); ++it)
    {
		if ((*it)->getSocket() == fd)
			return (*it);
	}

	return (NULL);
}

ClientData	*Server::find_ClientData_Socket_login(int fd)
{
    for (std::vector<ClientData*>::iterator it = clients_vec_login.begin(); it != clients_vec_login.end(); ++it)
    {
		if ((*it)->getSocket() == fd)
			return (*it);
	}
	return (NULL);
}


std::string getIP()
{
    char hostname[256];
    std::string ip;

    if (gethostname(hostname, sizeof(hostname)) == 0) 
    {
        struct hostent *hostinfo;

        if ((hostinfo = gethostbyname(hostname)) != NULL) 
            ip = inet_ntoa(*(struct in_addr *)hostinfo->h_addr);
        else 
        {
            std::cerr << "Error getting host IP address" << std::endl;
            return "";
        }
    } 
    else 
    {
        std::cerr << "Error getting hostname" << std::endl;
        return "";
    }

    return ip;
}


int Server::create_serversocket()
{
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        std::istringstream ss(_port);
        uint16_t valorHost;

        if (!(ss >> valorHost)) 
        {
            std::cerr << "Error: Could not convert string to uint16_t" << std::endl;
            CloseServer();
        } 
        const int reuse = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
        {
            std::cerr << "Error at setsocketopt(): " << std::endl;
            CloseServer();
        }
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(valorHost);
        std::string ip = getIP();
        addr.sin_addr.s_addr = INADDR_ANY;

        std::cout << BLUE << "Local IP: " << getIP() << NOCOLOR << std::endl;
        int bindResult = bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));
        if (bindResult == -1)
            perror("bindResult");
        int listenResult = listen(server_socket, 5);
        if (listenResult == -1)
            perror("listenResult");
        std::cout << GREEN << "Server start" << NOCOLOR << std::endl;
        return server_socket;
}

std::string	makeUserMsg(ClientData *user, std::string code, std::string input)
{
	std::ostringstream 	message;
	message << ":" << user->getHostname() << " " << code << " " << user->getNickName() << " :" << input << "\r\n";
	return (message.str());
}

std::string	makeUserMsg01(ClientData *user, std::string input)
{
	std::ostringstream 	message;
	message << ":" << user->getHostname() << " " << user->getNickName() << " :" << input << "\r\n";
	return (message.str());
}

void	sendToUser(ClientData *targetUser, std::string message)
{
	std::ostringstream debug;
	if (send(targetUser->getFd(), message.c_str(), message.size(), 0) < 0)
		throw std::invalid_argument(" > Error at sendToUser() ");
}


void	Server::deleteClient(size_t socket_num, ClientData *it_client)
{
    if (it_client != NULL)
    {
        std::cerr << RED << "Client disconnected" << NOCOLOR << std::endl;
        // Eliminar el socket del vector _sockets
        for (std::vector<pollfd>::iterator it = _sockets.begin(); it != _sockets.end(); ++it)
        {
            if (it_client->getSocket() == (*it).fd)
            {
                close((*it).fd);
                _sockets.erase(it);
                break;
            }
        }
        // Eliminar al usuario de los canales
        for (std::vector<ChannelData*>::iterator it = channel_vec.begin(); it != channel_vec.end();)
        {
            if ((*it)->deleteUser(it_client))
            {
                delete *it;
                it = channel_vec.erase(it); // Erase devuelve el siguiente iterador
            }
            else
            {
                ++it;
            }
        }
        // Eliminar el cliente del vector clients_vec
        for (std::vector<ClientData*>::iterator it = clients_vec.begin(); it != clients_vec.end(); ++it)
        {
            if (*it == it_client)
            {
                delete *it;
                clients_vec.erase(it);
                break;
            }
        }
        // Eliminar el cliente del vector clients_vec_login
        for (std::vector<ClientData*>::iterator it = clients_vec_login.begin(); it != clients_vec_login.end(); ++it)
        {
            if ((*it)->getSocketNum() == (int)socket_num)
            {
                delete *it;
                clients_vec_login.erase(it);
                break;
            }
        }
    }
}

std::string	Server::makePrivMsg(ClientData *sender, ClientData *receiver , std::string input)
{
	std::ostringstream 	message;
    if(input[0] != ':')
    {
        message << ":" << sender->getNickName() << " PRIVMSG " <<  receiver->getNickName() << " :" << input << "\r\n";
    }
    else
    {
        message << ":" << sender->getNickName() << " PRIVMSG " <<  receiver->getNickName() << " " << input << "\r\n";
    }
	return (message.str());
}

void Server::send_PersonalMessage(ClientData *sender)
{
    std::string name = args[1];
    std::string message;
    for (size_t i = 2; i < args.size(); ++i) 
    {
        message += args[i];
        if (i < args.size() - 1) {
            message += " ";
        }
    }

    for (std::vector<ClientData*>::iterator it = clients_vec.begin(); it != clients_vec.end(); ++it)
    {
        if ((*it)->getNickName() == name)
        {
            ClientData *receiver = *it;
            sendToUser(receiver, makePrivMsg(sender, receiver, message));
            return ;
        } 
    }
    sendToUser(sender, makeUserMsg(sender, ERR_ERRONEUSNICKNAME, "The client you want to send the message to does not exist"));
    return ;
}

void Server::sendWelcomeMessageToUser(ClientData* client) 
{
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "____________________________________________________________________"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "/                                                                      \\"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|              Welcome to our IRC server!                              |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|                                                                      |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "| We're excited to have you join us in our vibrant community. Here's   |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "| a brief overview of the five default channels you can explore:       |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|                                                                      |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "| 1. #ALL - Topic: A public channel where everyone is welcome to       |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|    engage in discussions on various topics.                          |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|                                                                      |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "| 2. #NEWS - Topic: Stay updated with current news and events.         |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|                                                                      |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "| 3. #CARS - Topic: Dive into discussions about automobiles and        |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|    everything related to cars.                                       |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|                                                                      |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "| 4. #MUSIC - Topic: Explore the latest music releases and share your  |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|    favorite tunes.                                                   |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|                                                                      |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "| 5. #GAMES - Topic: Join the gaming community to discuss video games  |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|    and the latest releases.                                          |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|                                                                      |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "| 6. #MOVIES - Topic: Delve into conversations about classic films     |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|    and cinematic masterpieces.                                       |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|                                                                      |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "| Feel free to hop into any channel that interests you and start       |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "| chatting. NOTE THAT THE SERVER CHANELS AND TOPICS MAY CHANGE!!!!!    |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "|                                                                      |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "\\____________________________________________________________________/"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "          \\"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "           \\"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "            \\   ^__^"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "             \\  (oo)\\_______"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "                (__)\\       )\\/\\"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "                    ||----w |"));
    sendToUser(client, makeUserMsg(client, RPL_WELCOME, "                    ||     ||"));
}


std::vector<std::string>	Server::splitString(std::string str, const char *dlmtrs)
{
	char	*ptr = strtok((char *)str.c_str(), dlmtrs);
    std::vector<std::string>().swap(args);
    args.clear();
	while (ptr != NULL && !std::string(ptr).empty())
	{
		args.push_back(std::string(ptr));
		ptr = strtok(NULL, dlmtrs);
	}

	return args;
}

ChannelData	*Server::findChannel(std::string str)
{
    for (std::vector<ChannelData*>::iterator it = this->channel_vec.begin(); it != this->channel_vec.end(); it++)
    {
        if ((*it)->getChannelName() == str)
            return(*it);
    }
	return (NULL);
}

void	Server::processChanMsg(ClientData *sender)
{
	ChannelData *chan = findChannel(args[1]);
    std::string message;
    if(!sender)
        return;
    if (args.size() < 3) {
        sendToUser(sender, makeUserMsg(sender, ERR_NEEDMOREPARAMS, "Not enough arguments"));
        return;
    }
    if(args[2][0] != ':')
        args[2] = ":" + args[2];
    for (size_t i = 0; i < args.size(); ++i) 
    {
        message += args[i];
        if (i < args.size() - 1) 
        {
            message += " ";
        }
    }
	if (chan == NULL)
		sendToUser(sender, makeUserMsg(sender, ERR_NOSUCHCHANNEL, "Channel does not exist"));
	else
	{
		if (!chan->hasMember(sender))
			sendToUser(sender, makeUserMsg(sender, ERR_CANNOTSENDTOCHAN, "You are not in this channel"));
		else
			chan->sendToChannel(sender, makeChanMsg(sender, message), false);
	}
}

std::string	makeChanMsg(ClientData *client, std::string input)
{
	std::ostringstream 	message;
    message << ":" << client->getNickName() << "!" << client->getLoginName() << "@" << getIP() << " " << input << "\r\n";
	return (message.str());
}

std::string	makeChanMsg(ClientData *client, std::string code, std::string input)
{
	std::ostringstream 	message;
    message << ":" << client->getNickName() << "!" << client->getLoginName() << "@" << getIP() << " " << code << " " << input << "\r\n";
	return (message.str());
}
