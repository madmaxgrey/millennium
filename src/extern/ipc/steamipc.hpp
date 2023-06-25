#pragma once
using tcp = boost::asio::ip::tcp;
using namespace boost::asio;
using namespace boost::asio::ip;

skin_config skinConfig;
namespace websocket = boost::beast::websocket;

/// <summary>
/// add functionality to the settings modal with usermode code execution
/// recieved from the steam_to_millennium_ipc
/// </summary>
class functions
{
public:
    steam_js_context js_context;
    /// <summary>
    /// update skin selection, requires skin name, and it to exist
    /// </summary>
    /// <param name="skin_update_name"></param>
    inline void update_skin(std::string skin_update_name)
    {
        std::string disk_path = std::format("{}/{}/skin.json", skinConfig.get_steam_skin_path(), skin_update_name);

        //whitelist the default skin
        if (skin_update_name == "default" || std::filesystem::exists(disk_path)) 
        {
            //update registry key holding the active-skin selection
            registry::set_registry("active-skin", skin_update_name);
            skin_config::skin_change_events::get_instance().trigger_change();

            js_context.reload();
        }
        else {
            MessageBoxA(
                GetForegroundWindow(), 
                std::format("file: {}\nin the selected skin was not found, therefor can't be loaded.", disk_path).c_str(), "Millennium",
                MB_ICONINFORMATION
            );
        }
    }

    const inline void respond_skins(websocket::stream<tcp::socket>& ws)
    {
        //get steam skins path
        std::string steam_skin_path = skinConfig.get_steam_skin_path();

        nlohmann::json data, skins;

        //search all folders in the skins directory and assume they are valid skin
        //it will stop the user if its not valid once they select it
        for (const auto& entry : std::filesystem::directory_iterator(steam_skin_path)) {
            if (entry.is_directory()) {
                skins.push_back({ {"name", entry.path().filename().string()} });
            }
        }
        //add the default skin virtually
        skins.push_back({ {"name", "default"} });

        data["skins"] = skins;
        //include other information used by the modal
        data["active-skin"] = registry::get_registry("active-skin");
        data["allow-javascript"] = registry::get_registry("allow-javascript");

        ws.write(boost::asio::buffer(data.dump()));
    }

    /// <summary>
    /// open the skins folder
    /// </summary>
    inline void open_skin_folder()
    {
        ShellExecute(NULL, "open", std::string(skinConfig.get_steam_skin_path()).c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    /// <summary>
    /// change javascript execution, allow or disallow
    /// </summary>
    /// <param name="value">true or false</param>
    inline void allow_javascript(bool value)
    {
        registry::set_registry("allow-javascript", value ? "true" : "false");
        //reload the steam interface to effectively restart the skinning
        js_context.reload();
    }
};

/// <summary>
/// inter program communication between the js on the steam webpage back to c++ here
/// </summary>
class millennium_ipc_listener {
public:
    millennium_ipc_listener(boost::asio::io_context& ioc) :
        acceptor_(ioc, tcp::endpoint(tcp::v4(), 3242)),
        socket_(ioc) {
        ipc_do_accept();
    }

private:
    void ipc_do_accept() {
        //start listening for a new message
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
            if (!ec) {
                std::make_shared<millennium_ipc_session>(std::move(socket_))->ipc_initialize();
            }
            ipc_do_accept();
        });
    }

    class millennium_ipc_session : public std::enable_shared_from_this<millennium_ipc_session> {
    public:
        millennium_ipc_session(tcp::socket socket) : ws_(std::move(socket)) {}

        void ipc_initialize() {
            //initial socket startup
            ws_.async_accept([self = shared_from_this()](boost::system::error_code ec) {
                if (!ec) {
                    self->read_request_payload();
                }
            });
        }

        void read_request_payload() 
        {
            //async read from the socket and wait for a message
            ws_.async_read(buffer_, [self = shared_from_this()](boost::system::error_code read_error, std::size_t bytes_transferred) 
            {
                if (read_error) return;

                functions* skin_helpers = new functions;

                //parse the message and convert it to the enum type
                nlohmann::basic_json<> msg = nlohmann::json::parse(boost::beast::buffers_to_string(self->buffer_.data()));
                ipc_types ipc_message = static_cast<ipc_types>(msg["type"].get<int>());

                //msg["content"] contains the payload from js, its a dynamic typing
                switch (ipc_message)
                {
                    case ipc_types::open_skins_folder: { skin_helpers->open_skin_folder(); break; }
                    case ipc_types::open_url: { ShellExecuteA(NULL, "open", msg["content"].get<std::string>().c_str(), 0, 0, 1); break; }
                    case ipc_types::change_javascript: { skin_helpers->allow_javascript(msg["content"].get<bool>()); break; }
                    case ipc_types::get_skins: { skin_helpers->respond_skins(self->ws_); break; }
                    case ipc_types::skin_update: { skin_helpers->update_skin(msg["content"]); break; }
                    case ipc_types::exec_command: { self->ws_.write(boost::asio::buffer(skin_helpers->js_context.exec_command(msg["content"]))); break; }
                }

                delete skin_helpers;
                self->buffer_.consume(bytes_transferred);
                self->read_request_payload();
            });
        }

    private:
        //ipc types, enumerated respectively
        enum ipc_types {
            open_skins_folder, skin_update, open_url, change_javascript, change_console, get_skins, exec_command
        };

        websocket::stream<tcp::socket> ws_;
        boost::beast::multi_buffer buffer_;
    };

    tcp::acceptor acceptor_;
    tcp::socket socket_;
};