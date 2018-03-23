#include <exporting.h>
#include <soundprocessing.h>

Exporting::Exporting(SoundProcessing* soundprocessing, QObject *parent) : QObject(parent)
{
 this->soundProc = soundprocessing;
}
//---------------------------------------------------
void Exporting::exportFile(QString filename){

    QFile* file= new QFile(filename);

    file->open(QIODevice::WriteOnly);
    //write WAVE header format to file
    writeWavHeader(file);
    //write recorded data
    file->write(soundProc->output1);
    closeWavHeader(file);

    emit exportReady();
}
//---------------------------------------------------
void Exporting::writeWavHeader( QFile * file )
{
    QDataStream out(file);
    out.setByteOrder(QDataStream::LittleEndian);

    // RIFF chunk
    out.writeRawData("RIFF", 4);
    out << quint32(0); // Placeholder for the RIFF chunk size (filled by close())
    out.writeRawData("WAVE", 4);

    // Format description chunk
    out.writeRawData("fmt ", 4);
    out << quint32(16); // "fmt " chunk size (always 16 for PCM)
    out << quint16(1); // data format (1 => PCM)
    out << quint16(soundProc->format.channelCount());
    out << quint32(soundProc->format.sampleRate());
    out << quint32(soundProc->format.sampleRate() * soundProc->format.channelCount()
                   * soundProc->format.sampleSize() / 8 ); // bytes per second
    out << quint16(soundProc->format.channelCount() * soundProc->format.sampleSize() / 8); // Block align
    out << quint16(soundProc->format.sampleSize()); // Significant Bits Per Sample

    // Data chunk
    out.writeRawData("data", 4);
    out << quint32(0); // Placeholder for the data chunk size (filled by close())

}
//---------------------------------------------------
void Exporting::closeWavHeader( QFile * file) {
    // Fill the header size placeholders
    quint32 fileSize = file->size();

    QDataStream out(file);
    // Set the same ByteOrder like in writeHeader()
    out.setByteOrder(QDataStream::LittleEndian);
    // RIFF chunk size
    file->seek(4);
    out << quint32(fileSize - 8);

    // data chunk size
    file->seek(40);
    out << quint32(fileSize - 44);

    file->close();
}
