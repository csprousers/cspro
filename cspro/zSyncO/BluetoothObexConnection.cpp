#include "stdafx.h"
#include "BluetoothObexConnection.h"
#include "IBluetoothAdapter.h"
#include "IObexTransport.h"
#include "ObexClient.h"
#include "IDataChunk.h"
#include <zNetwork/HeaderList.h>
#include <sstream>


namespace
{
    ObexHeaderList httpHeadersToObexHeaders(const HeaderList& httpHeaders)
    {
        ObexHeaderList obexHeaders;

        for( const std::string& header : httpHeaders.GetHeaders() )
            obexHeaders.add(ObexHeader(OBEX_HEADER_HTTP, header.c_str(), header.length()));

        return obexHeaders;
    }

    const HeaderList obexHeadersToHttpHeaders(ObexHeaderList& obexHeaders)
    {
        HeaderList httpHeaders;
        for (std::vector<ObexHeader>::const_iterator ih = obexHeaders.getHeaders().begin(); ih != obexHeaders.getHeaders().end(); ++ih) {
            if (ih->getCode() == OBEX_HEADER_HTTP) {
                httpHeaders.Add(std::string(ih->getByteSequenceData(), ih->getByteSequenceDataSize()));
            }
        }
        return httpHeaders;
    }
}


BluetoothObexConnection::BluetoothObexConnection(std::shared_ptr<IBluetoothAdapter> pAdapter)
    :   m_pAdapter(std::move(pAdapter))
{
}


BluetoothObexConnection::~BluetoothObexConnection()
{
    m_pObexClient.reset();
    if (m_pTransport) {
        m_pTransport->close();
    }
}


bool BluetoothObexConnection::connect(const BluetoothDeviceInfo& deviceInfo)
{
    m_pTransport = m_pAdapter->ConnectToRemoteDevice(deviceInfo.name, deviceInfo.address, OBEX_SYNC_SERVICE_UUID, m_syncListener.get());
    if (!m_pTransport)
        return false;

    m_pObexClient = std::make_unique<ObexClient>(m_pTransport.get());
    m_pObexClient->SetSyncListener(m_syncListener);

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
    const std::string type_utf8 = UTF8_TODO::GetUtf8(type);
    headers.add(ObexHeader(OBEX_HEADER_TYPE, type_utf8.c_str(), type_utf8.length()));

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
    const std::string type_utf8 = UTF8_TODO::GetUtf8(type);
    headers.add(ObexHeader(OBEX_HEADER_TYPE, type_utf8.c_str(), type_utf8.length()));

    ObexHeaderList responseObexHeaders;
    ObexResponseCode putResult = m_pObexClient->put(headers, content, response, responseObexHeaders);
    responseHeaders = obexHeadersToHttpHeaders(responseObexHeaders);
    return putResult;
}


IDataChunk& BluetoothObexConnection::getChunk()
{
    return m_pObexClient->getChunk();
}
