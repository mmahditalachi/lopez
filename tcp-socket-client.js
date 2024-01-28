const net = require("net");

const host = "127.0.0.1";
const port = 8989;
let firstChat = 0;
let chat_socket = 0;
let login_accpeted = 0;

const client = net.createConnection(port, host, () => {});

client.on("data", (data) => {
  console.log(`Received: ${data}`);
  let tokens = String(data).split("|");

  if (tokens[0] == "ok" && tokens[1] == "login") {
    login_accpeted = 1;
    firstChat = 1;
  } else if (tokens[0] == "error" && tokens[1] == "login") {
    login_accpeted = 0;
  } else if (tokens[0] == "error" && tokens[1] == "send") {
    firstChat = 1;
  } else if (tokens[0] == "error" && tokens[1] == "newsend") {
    firstChat = 1;
  } else if (tokens[0] == "ok" && tokens[1] == "send") {
    chat_socket = tokens[3];
    firstChat = 0;
  }

  console.log(`message: ${firstChat}`);
  console.log(`message: ${chat_socket}`);
});

client.on("error", (error) => {
  console.log(`Error: ${error.message}`);
});

client.on("close", () => {
  console.log("Connection closed");
});

function login(username, password) {
  client.write(`login|${username}|${password}`);
}
