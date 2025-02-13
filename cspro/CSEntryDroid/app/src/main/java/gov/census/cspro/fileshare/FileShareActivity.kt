package gov.census.cspro.fileshare

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import gov.census.cspro.csentry.CSEntryDirectory
import java.io.BufferedWriter
import java.io.OutputStreamWriter
import java.lang.Exception

class FileShareActivity: Activity() {
    private val commands: MutableMap<String, FileShareCommand> = mutableMapOf()
    private var csEntryDirectory: CSEntryDirectory? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        csEntryDirectory = CSEntryDirectory(this)
        buildCommandsMap()

        //getting command from intent
        val c = intent.getStringExtra("COMMAND")
        val res = Intent()
        if (commands.containsKey(c)) {
            //getting arguments
            val a = intent.getStringArrayExtra("ARGS")

            if (a != null) {
                try {
                    res.putExtra("RES", commands[c]?.runCommand(a))
                } catch (exc: Exception) {
                    //if command fails, returning the input arguments and an ERR extra string
                    res.putExtra("RES", a)
                    res.putExtra("ERR", exc.message)
                }
            }
        }

        setResult(RESULT_OK, res)
        finish()
    }

    //instantiating concrete command implementers
    private fun buildCommandsMap() {
        commands.clear()
        csEntryDirectory?.let { MakeDirFileShareCommand(this, it) }?.let {
            commands.put("MK_DIR",
                it
            )
        }
        csEntryDirectory?.let { ListDirFileShareCommand(this, it) }?.let {
            commands.put("LIST_DIR",
                it
            )
        }

        csEntryDirectory?.let { PushFilesFileShareCommand(this, it) }?.let {
            commands.put("PUSH_FILES",
                it
            )
        }

        csEntryDirectory?.let { PullFilesFileShareCommand(this, it) }?.let {
            commands.put("PULL_FILES",
                it
            )
        }

        csEntryDirectory?.let { DelFileShareCommand(this, it) }?.let {
            commands.put("DEL",
                it
            )
        }
    }
}