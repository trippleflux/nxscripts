[nxMyDB]
Host            = localhost     # MySQL Server host
Port            = 3306          # MySQL Server port
User            = user          # MySQL Server username
Password        = pass          # MySQL Server password
Database        = ioftpd        # Database name

# Connection type
Compression     = True          # Use compression for the server connection
Encryption      = True          # Use SSL encryption for the server connection

# Connection pools
Pool_Minimum     = 2            # Minimum number of sustained connections
Pool_Maximum     = 5            # Maximum number of sustained connections
Pool_Keep_Alive  = 28800        # Seconds to keep a connection alive (same as MySQL's interactive_timeout)
Pool_Timeout     = 5            # Seconds to wait for a connection to become available
