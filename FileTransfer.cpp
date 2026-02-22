#include "FileTransfer.h"
#include "Connection.h"
#include "SimpleMessage.h"
#include "MessageType.h"
#include "DataResource.h"
#include "Logger.h"
#include "StringUtils.h"
#include "DirectoryListing.h"

#include <filesystem>


void FileTransferHandler::OnFileTransferDataReceived(std::shared_ptr<FileTransfer> file_transfer,
                                          std::shared_ptr<Message> msg) {
}

void FileTransferHandler::OnFileTransferCompleted(std::shared_ptr<FileTransfer> file_transfer) {
  _transfers.erase(file_transfer->GetRequestId());
}

bool FileTransferHandler::MaybeReleaseTransferIfEnded(std::shared_ptr<FileTransfer> transfer) {
  if(transfer->HasFailed() || (transfer->GetExpectedFileSize() == transfer->GetReceivedFileSize())) {
    _transfers.erase(transfer->GetRequestId());
    return true;
  }
  return false;
}

void FileTransferHandler::ReleaseAllTransfers() {
  _transfers.clear();
}

FileTransfer::FileTransfer(std::weak_ptr<FileTransferHandler> listener
                          ,uint32_t req_id
                          ,const std::string& req_file_path
                          ,bool is_get_request)
    : _listener(listener)
    , _req_id(req_id)
    , _req_file_path(req_file_path)
    , _is_get_request(is_get_request)
    , _is_directory_listing_request(false)
    , _current_state(State::IDLE)
    , _received_file_size(0)
    , _expected_file_size(0)
    , _data_transfer_counter(0) {
}

bool FileTransfer::SwitchState(FileTransfer::State new_state) {
  bool rejected = true;
  switch(_current_state) {
    case FileTransfer::State::IDLE:
      {
        if(new_state == FileTransfer::State::AWAITING_INIT_MSG ||
            new_state == FileTransfer::State::AWAITING_ACK_MSG) {
          rejected = false;
        }
      }
      break;
    case FileTransfer::State::AWAITING_INIT_MSG:
      {
        if(new_state == FileTransfer::State::RECEIVING_DATA) {
          rejected = false;
        }
      }
      break;
    case FileTransfer::State::AWAITING_ACK_MSG:
      {
        if(new_state == FileTransfer::State::SENDING_DATA) {
          rejected = false;
        }
      }
    case FileTransfer::State::SENDING_DATA:
    case FileTransfer::State::RECEIVING_DATA:
    case FileTransfer::State::DONE:
      {
        if(new_state == FileTransfer::State::DONE) {
          rejected = false;
        }
      }
      break;
    default:
      break;
  }

  if(rejected) {
    DLOG(error, "Invalid transition from current state : {} to new state", (int)_current_state, (int)new_state);
    OnFail();
    return false;
  } else {
    _current_state = new_state;
  }
  return true;
}

uint32_t FileTransfer::GetRequestId() {
  return _req_id;
}

const std::string& FileTransfer::GetRequestPath() {
  return _req_file_path;
}

uint32_t FileTransfer::GetDataTransferCounter() {
  return _data_transfer_counter;
}

uint64_t FileTransfer::GetExpectedFileSize() {
  return _expected_file_size;
}

uint64_t FileTransfer::GetReceivedFileSize() {
  return _received_file_size;
}

void FileTransfer::SendTransferRequestMsg(std::shared_ptr<Client> client) {
  if(!SwitchState(FileTransfer::State::AWAITING_INIT_MSG)) {
    return;
  }

  uint32_t data_size = 4 + 1 + _req_file_path.length();
  auto data = std::make_shared<Data>(data_size);
  data->Add(4, (unsigned char*)&_req_id);
  data->Add(1, (unsigned char*)&_is_get_request);
  data->Add(_req_file_path.length(), (unsigned char*)_req_file_path.c_str());

  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::FILE_TRANSFER_REQ, resource);
  client->Send(msg);
}

void FileTransfer::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  if(_current_state != FileTransfer::State::RECEIVING_DATA) {
    std::shared_ptr<SimpleMessage> simple_msg = std::static_pointer_cast<SimpleMessage>(msg);
    auto msg_header = simple_msg->GetHeader();
    auto type = MessageType::TypeFromInt(msg_header->_type);
    switch(type) {
      case MessageType::FILE_TRANSFER_ACK:
        SendRequestedData();
        break;
      default:
        {
          DLOG(error, "OnClientRead : unexpected message type : {}", msg_header->_type);
          OnFail();
        }
        break;
    }
  } else {
    HandleFileTransferMsg(msg);
  }
}

bool FileTransfer::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  if(err != NetError::OK) {
    OnFail();
    return false;
  }

  auto msg_builder = std::unique_ptr<SimpleMessageBuilder>(new SimpleMessageBuilder());
  client->SetMsgBuilder(std::move(msg_builder));
  return true;
}

void FileTransfer::OnClientConnected(std::shared_ptr<Client> client) {
  _client = client;
  SendInitResponse();
}

void FileTransfer::OnClientClosed(std::shared_ptr<Client> client) {
  if(_current_state != FileTransfer::State::DONE && _current_state != FileTransfer::State::FAILED) {
    OnFail();
  }
}

void FileTransfer::OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) {
  if(_current_state != FileTransfer::State::SENDING_DATA) {
    return;
  }
  SwitchState(FileTransfer::State::DONE);
  auto listener = _listener.lock();
  if(listener) {
    listener->OnFileTransferCompleted(shared_from_this());
  } else {
    DLOG(error, "Cant' lock listener");
  }
}

void FileTransfer::SendInitResponse() {
  if(!SwitchState(FileTransfer::State::AWAITING_ACK_MSG)) {
    return;
  }

  bool is_valid = true;
  uint64_t file_length = 0;
  std::error_code fs_error;
  if(_is_get_request) {
    std::filesystem::path path(_req_file_path);
    bool path_exists = std::filesystem::exists(path, fs_error);
    if (!path_exists || fs_error) {
      DLOG(error, "Path can't be accessed : {}", _req_file_path);
      is_valid = false;
    } else {
      bool is_dir = std::filesystem::is_directory(path, fs_error);
      if(fs_error) {
        DLOG(error, "is_directory failed on : {}", _req_file_path);
        is_valid = false;
      } else {
        if(is_dir) {
          _serialized_dir = DirectoryListing::SerializeDirectory(_req_file_path);
          if(!_serialized_dir) {
            DLOG(error, "Serialize directory failed on : {}", _req_file_path);
            file_length = 0;
          } else {
            _is_directory_listing_request = true;
            file_length = _serialized_dir->GetCurrentSize();
          }
        } else {
          file_length = (uint64_t)std::filesystem::file_size(path, fs_error);
          if(fs_error) {
            DLOG(error, "file_size failed on : {}", _req_file_path);
            is_valid = false;
            file_length = 0;
          }
        }
      }
    }
  }

  uint32_t data_size = 4 + 1 + 1 + 8;
  auto data = std::make_shared<Data>(data_size);
  data->Add(4, (unsigned char*)&_req_id);
  data->Add(1, (unsigned char*)&is_valid);
  data->Add(1, (unsigned char*)&_is_directory_listing_request);
  data->Add(8, (unsigned char*)&file_length);
  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::FILE_TRANSFER_INIT, resource);

  _client->Send(msg);

  if(!is_valid) {
    OnFail();
  }
}

void FileTransfer::HandleTransferInit(std::shared_ptr<Client> client, std::shared_ptr<Data> data) {
  if(!SwitchState(FileTransfer::State::RECEIVING_DATA)) {
    return;
  }


  _client = client;
  uint32_t req_id = 0;
  uint8_t is_valid = 0;
  uint8_t is_directory_listing = 0;
  uint64_t file_size = 0;
  bool data_retrieved = true;

  data_retrieved = data_retrieved && data->CopyTo(&req_id, 0, 4);
  data_retrieved = data_retrieved && data->CopyTo(&is_valid, 4, 1);
  data_retrieved = data_retrieved && data->CopyTo(&_is_directory_listing_request , 5, 1);
  data_retrieved = data_retrieved && data->CopyTo(&_expected_file_size , 6, 8);

  if(!data_retrieved) {
    DLOG(error, "HandleTransferInit : data read error");
    return;
  }

  if(!is_valid) {
    //TODO
    OnFail();
    return;
  }

  if(!_is_get_request) {
    //TODO
    DLOG(error, "Got unsupported non get request");
    OnFail();
    return;
  }

  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::FILE_TRANSFER_ACK);
  _client->SetMsgBuilder(nullptr);
  _client->Send(msg);
}

void FileTransfer::OnFail() {
  _current_state = FileTransfer::State::FAILED;
  auto listener = _listener.lock();
  if(listener) {
    listener->OnFileTransferFailed(shared_from_this());
  } else {
    DLOG(error, "OnFail : cant' lock listener");
  }
}

void FileTransfer::HandleFileTransferMsg(std::shared_ptr<Message> msg) {
  _data_transfer_counter++;
  _received_file_size +=  msg->GetDataResource()->GetSize();

  if(_received_file_size == _expected_file_size) {
    SwitchState(FileTransfer::State::DONE);
  }

  auto listener = _listener.lock();
  if(listener) {
    listener->OnFileTransferDataReceived(shared_from_this(), msg);
    if(_current_state == FileTransfer::State::DONE) {
      listener->OnFileTransferCompleted(shared_from_this());
    }
  } else {
    DLOG(error, "HandleFileTransferDataMsg : cant' lock listener");
  }
}

void FileTransfer::SendRequestedData() {
  if(!SwitchState(FileTransfer::State::SENDING_DATA)) {
    return;
  }

  std::error_code fs_error;
  std::filesystem::path path(_req_file_path);
  std::shared_ptr<Message> content_msg;

  bool is_dir = std::filesystem::is_directory(path, fs_error);
  if(fs_error) {
    DLOG(error, "is_directory failed on : {}", _req_file_path);
  }

  if(is_dir && _serialized_dir) {
    auto resource = std::make_shared<DataResource>(_serialized_dir);
    content_msg = std::make_shared<Message>(resource);
  } else {
    auto file_resource = DataResource::CreateFromFile(_req_file_path);
    if(file_resource != nullptr) {
      content_msg = std::make_shared<Message>(file_resource);
    }
  }
  _client->Send(content_msg);
}

bool FileTransfer::IsGetRequest() {
  return _is_get_request;
}

bool FileTransfer::HasFailed() {
  return (_current_state == State::FAILED);
}

bool FileTransfer::IsDirectoryListingRequest() {
  return _is_directory_listing_request;
}

bool FileTransfer::SaveToOutputDirectory(std::shared_ptr<SimpleMessage> msg, const std::string& dir_path) {
  std::string file_name;
  std::vector<std::string> split = StringUtils::Split(_req_file_path, "/");
  if(split.size()) {
    file_name = split.back();
  }
  if(file_name.empty()) {
    return false;
  }

  std::shared_ptr<DataResource> resource = msg->GetContent();
  return resource->SaveToFile(dir_path + "/" + file_name);
}