#include "stdafx.h"
#include <sstream>
#include "BluetoothObexConnection.h"
#include "IObexTransport.h"
#include "IBluetoothAdapter.h"
#include "ObexClient.h"
#include "ObexConstants.h"
#include "SyncException.h"
#include "IDataChunk.h"

namespace {

    ObexHeaderList httpHeadersToObexHeaders(const HeaderList& httpHeaders)
    {
        ObexHeaderList obexHeaders;
        for (HeaderList::const_iterator ih = httpHeaders.begin(); ih != httpHeaders.end(); ++ih) {
            std::string hUtf8 = UTF8Convert::WideToUTF8(*ih);
            obexHeaders.add(ObexHeader(OBEX_HEADER_HTTP, hUtf8.c_str(), hUtf8.length()));
        }
        return obexHeaders;
    }

    const HeaderList obexHeadersToHttpHeaders(ObexHeaderList& obexHeaders)
    {
        HeaderList httpHeaders;
        for (std::vector<ObexHeader>::const_iterator ih = obexHeaders.getHeaders().begin(); ih != obexHeaders.getHeaders().end(); ++ih) {
            if (ih->getCode() == OBEX_HEADER_HTTP) {
                httpHeaders.push_back(UTF8Convert::UTF8ToWide<CString>(ih->getByteSequenceData(), (int) ih->getByteSequenceDataSize()));
            }
        }
        return httpHeaders;
    }
}

BluetoothObexConnection::BluetoothObexConnection(IBluetoothAdapter* pAdapter)
    : m_pAdapter(pAdapter),
      m_pTransport(NULL),
      m_pObexClient(NULL),
      m_pListener(NULL)
{
}

BluetoothObexConnection::~BluetoothObexConnection()
{
    delete m_pObexClient;
    if (m_pTransport) {
        m_pTransport->close();
        delete m_pTransport;
    }
}

bool BluetoothObexConnection::connect(const BluetoothDeviceInfo& deviceInfo)
{
    m_pTransport = m_pAdapter->connectToRemoteDevice(deviceInfo.csName, deviceInfo.csAddress,
        OBEX_SYNC_SERVICE_UUID, m_pListener);
    if (!m_pTransport)
        return false;

    m_pObexClient = new ObexClient(m_pTransport);
    m_pObexClient->setListener(m_pListener);

    ObexHeaderList headers;
    ObexHeader targetHeader(OBEX_HEADER_TARGET,
        reinterpret_cast<const char*>(OBEX_FOLDER_BROWSING_UUID),
        sizeof(OBEX_FOLDER_BROWSING_UUID));
    headers.add(targetHeader);

    // Pass client Bluetooth protocol version to server, so compatibility can be detected
    ObexHeader bluetoothProtocolVersionHeader(OBEX_HEADER_BLUETOOTH_PROTOCOL_VERSION, BLUETOOTH_PROTOCOL_VERSION);
    headers.add(bluetoothProtocolVersionHeader);

    ObexResponseCode connectResult = m_pObexClient->connect(headers);

    return connectResult == OBEX_OK;
}

void BluetoothObexConnection::disconnect()
{
    ObexHeaderList headers;
    m_pObexClient->disconnect(headers);
}

ObexResponseCode BluetoothObexConnection::get(CString type, CString path, const HeaderList& requestHeaders,
    std::ostream& response, HeaderList& responseHeaders)
{
    ObexHeaderList headers;
    headers.add(ObexHeader(OBEX_HEADER_NAME, path));

    // For some strange reason OBEX wants the type to be ASCII text
    // in binary format instead of in unicode string format.
    std::string typeUtf8 = UTF8Convert::WideToUTF8(type);
    headers.add(ObexHeader(OBEX_HEADER_TYPE, &typeUtf8[0], typeUtf8.size()));

    headers.add(httpHeadersToObexHeaders(requestHeaders));

    ObexHeaderList responseObexHeaders;
    ObexResponseCode responseCode = m_pObexClient->get(headers, response, responseObexHeaders);
    responseHeaders = obexHeadersToHttpHeaders(responseObexHeaders);
    return responseCode;
}

ObexResponseCode BluetoothObexConnection::put(CString type, CString path, bool isLastFileChunk, std::istream& content, size_t contentLength,
    const HeaderList& requestHeaders, std::ostream& response, HeaderList& responseHeaders)
{
    ObexHeaderList headers;
    headers.add(ObexHeader(OBEX_HEADER_NAME, path));
    headers.add(ObexHeader(OBEX_HEADER_IS_LAST_FILE_CHUNK, static_cast<int>(isLastFileChunk)));
    headers.add(ObexHeader(OBEX_HEADER_LENGTH, (int) contentLength));
    headers.add(httpHeadersToObexHeaders(requestHeaders));

    // For some strange reason OBEX wants the type to be ASCII text
    // in binary format instead of in unicode string format.
    std::string typeUtf8 = UTF8Convert::WideToUTF8(type);
    headers.add(ObexHeader(OBEX_HEADER_TYPE, &typeUtf8[0], typeUtf8.size()));

    ObexHeaderList responseObexHeaders;
    ObexResponseCode putResult = m_pObexClient->put(headers, content, response, responseObexHeaders);
    responseHeaders = obexHeadersToHttpHeaders(responseObexHeaders);
    return putResult;
}

IDataChunk& BluetoothObexConnection::getChunk()
{
    return m_pObexClient->getChunk();
}

void BluetoothObexConnection::setListener(ISyncListener* pListener)
{
    m_pListener = pListener;
}

