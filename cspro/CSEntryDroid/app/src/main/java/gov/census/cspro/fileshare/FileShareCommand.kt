package gov.census.cspro.fileshare

import android.content.res.Resources
import gov.census.cspro.csentry.CSEntryDirectory
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.EngineInterface
import java.io.*
import java.util.zip.ZipEntry
import java.util.zip.ZipInputStream
import java.util.zip.ZipOutputStream

//Base class for file share server command implementers
abstract class FileShareCommand(activity: FileShareActivity, csEntryDirectory: CSEntryDirectory) {
    protected val activity = activity
    protected val csEntryDirectory = csEntryDirectory

    abstract fun runCommand(args: Array<String>): Array<String>
}

//Make directory command
class MakeDirFileShareCommand(activity: FileShareActivity, csEntryDirectory: CSEntryDirectory)
    :FileShareCommand(activity, csEntryDirectory) {

    override fun runCommand(args: Array<String>): Array<String> {
        val dir = args[0]

        //making directory if it doesn't exist already
        File(csEntryDirectory.path, dir).mkdirs()

        return args
    }
}

//List directories command
class ListDirFileShareCommand(activity: FileShareActivity, csEntryDirectory: CSEntryDirectory)
    :FileShareCommand(activity, csEntryDirectory) {
    override fun runCommand(args: Array<String>): Array<String> {

        val files = File(csEntryDirectory.path, args[0])
            .listFiles()

        var res = mutableListOf<String>()
        files.forEach {
            res.add("${it.name}${
                if (it.isDirectory) "/"
                else ""
            }")
        }

        return  res.toTypedArray()
    }
}

//Push files fot 'csentry/' from client command
class PushFilesFileShareCommand(activity: FileShareActivity, csEntryDirectory: CSEntryDirectory)
    :FileShareCommand(activity, csEntryDirectory) {
    override fun runCommand(args: Array<String>): Array<String> {

        val csEntryDirectory = File(csEntryDirectory.path)

        //file overwrite flag. true by default
        val overwrite = args.count() < 2 || !args[1].equals("false", true)

        //Converting input stream to Zip stream and extracting it
        activity.contentResolver.openInputStream(activity.intent.data!!).let {
            if (it != null) {
                //f.copyInputStreamToFile(it)
                val zis = ZipInputStream(BufferedInputStream(it))

                var entry: ZipEntry?

                // Read each entry from the ZipInputStream until no
                // more entry found indicated by a null return value
                // of the getNextEntry() method.
                while (zis.nextEntry.also { entry = it } != null) {
                    val ef = File(csEntryDirectory, entry?.name)

                    if (!ef.canonicalPath.startsWith(csEntryDirectory.canonicalPath)){
                        throw SecurityException(activity.getString(R.string.file_share_security_error))
                    }

                    val fileExists = ef.exists()
                    if (!overwrite && fileExists)
                        continue

                    //deleting the old file and/or creating directory
                    ef.parentFile.mkdirs()
                    if (fileExists)
                        ef.delete()

                    var size: Int
                    val buffer = ByteArray(2048)
                    FileOutputStream(ef).use { fos ->
                        BufferedOutputStream(fos, buffer.size).use { bos ->
                            while (zis.read(buffer, 0, buffer.size).also { size = it } != -1) {
                                bos.write(buffer, 0, size)
                            }
                            bos.close()
                        }
                    }
                }

                zis.close()
            }
        }

        return  args;
    }
}

//pull files by client from 'csentry/'
class PullFilesFileShareCommand(activity: FileShareActivity, csEntryDirectory: CSEntryDirectory)
    :FileShareCommand(activity, csEntryDirectory) {
    override fun runCommand(args: Array<String>): Array<String> {
        val csEntryDirectory = File(csEntryDirectory.path)
        val sourceDirectory = File(csEntryDirectory, args[1]);
        val deepSearch = args[4].equals("true", true)

        //getting files to write to output stream
        val regex = Regex(EngineInterface.CreateRegularExpressionFromFileSpec(args[2]))
        val files = mutableListOf<String>()

        if (deepSearch) {
            sourceDirectory.walk().filter {
                it.isFile && regex.matches(it.name)
            }.forEach {
                files.add(it.absolutePath)
            }
        }
        else {
            sourceDirectory.listFiles().filter {
                it.isFile && regex.matches(it.name)
            }.forEach {
                files.add(it.absolutePath)
            }
        }

        if (files.count() > 0) {
            activity.contentResolver.openOutputStream(activity.intent.data!!, "w").let {
                if (it != null) {
                    val zos = ZipOutputStream(BufferedOutputStream(it))

                    //writing files to output stream
                    files.forEach {
                        var fi = FileInputStream(it)
                        var origin = BufferedInputStream(fi)

                        //get entry name
                        val entryName = it.substring(sourceDirectory.absolutePath.length + 1)

                        var entry = ZipEntry(entryName)
                        zos.putNextEntry(entry)
                        origin.copyTo(zos, 1024)
                        origin.close()
                    }

                    zos.close()
                }
            }
        }

        return args;
    }
}

//deleting file of directory command
class DelFileShareCommand(activity: FileShareActivity, csEntryDirectory: CSEntryDirectory)
    :FileShareCommand(activity, csEntryDirectory) {

    override fun runCommand(args: Array<String>): Array<String> {
        val csEntryDirectory = File(csEntryDirectory.path)
        val sourceDirectory = File(csEntryDirectory, args[0]);
        val delFiles = args[2].equals("true", true)
        val delDirs = args[3].equals("true", true)

        //getting files to delete
        val regex = Regex(EngineInterface.CreateRegularExpressionFromFileSpec(args[1]))
        val files = mutableListOf<File>()

        sourceDirectory.listFiles().filter {
            ((delFiles && it.isFile) || (delDirs && it.isDirectory)) && regex.matches(it.name)
        }.forEach {
            files.add(it)
        }

        files.forEach {
            it.deleteRecursively()
        }

        return args
    }
}

