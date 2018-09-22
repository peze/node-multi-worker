{
  "targets": [
    {
      "target_name": "node-worker",
      "sources": [ "src/worker.cpp", "src/client.cpp","src/main.cpp"]
    }
  ],
  'cflags': [
	  '-Wall',
	  '-O3',
	  '-std=c++11'
  ]
}