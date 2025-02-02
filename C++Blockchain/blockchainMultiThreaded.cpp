// Includes
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <iterator>
#include <chrono>
#include <ctime>
#include <iomanip>
#include "sha256.h"

// Namespaces
using namespace std;

// Lock
mutex nonceLock;
int tempNonce = 0;

int getAndIncrementNonce()
{
    int currentNonce = tempNonce;
    tempNonce = currentNonce + 1;
    return currentNonce;
}

// Block class
class Block
{
public:
    // Block properties
    string data;
    string hash;
    string previousHash;
    long timeStamp;
    int nonce;
    bool foundHash;

    // Block constructor
    Block(string inputData, string inputPreviousHash, long inputTimeStamp)
    {
        this->data = inputData;
        this->previousHash = inputPreviousHash;
        this->timeStamp = inputTimeStamp;
        this->nonce = 0;
        this->hash = calculateBlockHash();
        this->foundHash = false;
    }

    // Standard getters
    string getHash()
    {
        return this->hash;
    }

    bool getFoundHash()
    {
        return this->foundHash;
    }

    string getPreviousHash()
    {
        return this->previousHash;
    }

    string getData()
    {
        return this->data;
    }

    long getTimeStamp()
    {
        return this->timeStamp;
    }

    int getNonce()
    {
        return this->nonce;
    }

    // Standard setters
    void setHash()
    {
        this->hash = calculateBlockHash();
    }

    void setFoundHash(bool input)
    {
        this->foundHash = input;
    }

    void setHashThreaded(string input)
    {
        this->hash = input;
    }

    void setPreviousHash(string inputHash)
    {
        this->previousHash = inputHash;
    }

    void setData(string inputData)
    {
        this->data = inputData;
    }

    void setTimeStamp(long inputTimeStamp)
    {
        this->timeStamp = inputTimeStamp;
    }

    void setNonce(int inputNonce)
    {
        this->nonce = inputNonce;
    }

    // Method to calculate block hash
    string calculateBlockHash()
    {
        // Concatenate parts of the block to generate the hash
        string dataToHash = this->previousHash + to_string(this->timeStamp) + to_string(this->nonce) + this->data;

        // Calculate hash based on block's data
        string hashedData = sha256(dataToHash);

        // Return hashed data
        return hashedData;
    }

    // Method to calculate block hash (Input parameters)
    string calculateBlockHashThreaded(string prevHash, long timeS, int non, string da)
    {
        // Concatenate parts of the block to generate the hash
        string dataToHash = prevHash + to_string(timeS) + to_string(non) + da;

        // Calculate hash based on block's data
        string hashedData = sha256(dataToHash);

        // Return hashed data
        return hashedData;
    }

    // Function that each thread runs
    void threadMineBlock(int threadID, int prefix)
    {
        // Prefix to look for in hash
        string prefixString(prefix, '0');

        // Search for hash with pre-determined prefix
        int i = 0;
        while(this->getFoundHash() == false)
        {
            // Get nonce
            nonceLock.lock();
            int localNonce = getAndIncrementNonce();
            nonceLock.unlock();

            // Calculate new hash
            string localHash = calculateBlockHashThreaded(this->previousHash, this->timeStamp, localNonce, this->data);

            // Check if hash contains the prefix
            if (localHash.substr(0, prefix) == prefixString && this->foundHash == false)
            {
                this->setFoundHash(true);
                this->setHashThreaded(localHash);
                this->setNonce(localNonce);
            }
        }
    }

    // Method to mine a block
    void mineBlock(int prefix)
    {
        // Get maximum number of threads the system can support
        int numThreads = thread::hardware_concurrency();

        // Create threads to mine block
        vector<thread> vecOfThreads;
        this->setFoundHash(false);
        tempNonce = 0;
        for (int i = 0; i < numThreads; i++)
        {
            thread threadObject(&Block::threadMineBlock, this, i, prefix);
            vecOfThreads.push_back(move(threadObject));
        }

        // wait for block to be mined
        while (foundHash == false)
        {
        }

        for (thread & th : vecOfThreads)
        {
            if (th.joinable())
            {
                th.join();
            }
        }

        // Finished mining the block
        return;
    }

    // Validate the block
    bool validateBlock(vector<Block> blockchain, int prefix)
    {
        // Initialize a boolean value to true
        bool valid = true;

        // The prefix string we want to match our hash prefix to
        string prefixString(prefix, '0');

        // Local variable to store the size of the blockchain
        int size = blockchain.size();

        // If this blockchain is NOT empty then we are NOT validating the genesis block
        if(size >= 1)
        {
            // Validate the previous hash, hash and prefix
            bool previousHashValid = this->getPreviousHash() == blockchain.at(size - 1).hash;
            bool hashValid = this->hash == calculateBlockHashThreaded(this->getPreviousHash(), this->getTimeStamp(), this->getNonce(), this->getData());
            // bool hashValid = this->hash == calculateBlockHash();
            valid = previousHashValid && hashValid;
        }
            // Otherwise it MUST be the genesis block since the blockchain is empty
        else
        {
            string emptyHash = "0000000000000000000000000000000000000000000000000000000000000000";
            bool previousHashValid = this->getPreviousHash() == emptyHash;
            bool hashValid = this->hash == calculateBlockHashThreaded(this->getPreviousHash(), this->getTimeStamp(), this->getNonce(), this->getData());
            // bool hashValid = this->hash == calculateBlockHash();
            valid = previousHashValid && hashValid;
        }

        // Return whether the block is valid or not.
        return valid;
    }
private:
};

// Main method
int main(int argc, char *argv[])
{
    // Get file path for input file
    string filePath = "../gorgias.txt";
    if (argv[1] != NULL)
    {
        filePath = argv[1];
    }

    // Get difficulty of block mining. Higher is harder (Default = 2)
    int prefix = 2;
    if (argv[2] != NULL)
    {
        prefix = atoi(argv[2]);
    }

    // Determine whether to output alldata or only test data (Default = test data only)
    bool fullOutput = false;
    if (argv[3] != NULL)
    {
        fullOutput = true;
    }

    // Create a vector of block times
    vector<float> executionTimes;

    // Start timer
    long startBlockchain = (chrono::duration_cast<chrono::microseconds>((chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())).time_since_epoch())).count();

    // Create blockchain (list of blocks)
    vector<Block> blockchain;

    // Grab the first file line as data for the genesis block
    ifstream inputFile;
    inputFile.open(filePath, ios::in);
    string data;
    if (inputFile.is_open())
    {
        getline(inputFile, data);
    }
    else
    {
        if (fullOutput)
        {
            cout << "[Error]: FILE IS NOT OPEN 223" << endl;
        }
    }

    // Instantiate the genesis block
    long currentDate = (chrono::duration_cast<chrono::milliseconds>((chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now())).time_since_epoch())).count();
    string emptyHash = "0000000000000000000000000000000000000000000000000000000000000000";
    Block genesisBlock(data, emptyHash, currentDate);

    if (fullOutput)
    {
        cout << "Creating genesis block" << endl;
    }

    // Mine the genesis block to create the hash for the next block.
    genesisBlock.mineBlock(prefix);

    if (fullOutput)
    {
        cout << "Validating genesis block" << endl;
    }

    // Validate our newly mined genesis block, using null as prev hash.
    while(!genesisBlock.validateBlock(blockchain, prefix))
    {
        // If our validation fails, retry mining after resetting values.
        genesisBlock.setNonce(0);
        long currentDate = (chrono::duration_cast<chrono::milliseconds>((chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now())).time_since_epoch())).count();
        genesisBlock.setTimeStamp(currentDate);
        genesisBlock.calculateBlockHash();
        genesisBlock.mineBlock(prefix);
    }

    // Add genesis block to the blockchain
    blockchain.push_back(genesisBlock);

    if (fullOutput)
    {
        cout << "Blockchain initialized" << endl << "Adding data blocks..." << endl;
    }

    // Continue adding blocks until we reach the end of the input file
    while(inputFile.peek() != EOF)
    {
        // Start timer for this block
        long startBlock = (chrono::duration_cast<chrono::microseconds>((chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())).time_since_epoch())).count();

        // Read in the next line
        if (inputFile.is_open())
        {
            getline(inputFile, data);
        }
        else
        {
            if (fullOutput)
            {
                cout << "[Error]: FILE IS NOT OPEN" << endl;
            }
        }

        // Create a new block with our line of data.
        long currentDate = (chrono::duration_cast<chrono::milliseconds>((chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now())).time_since_epoch())).count();
        Block newBlock(data, blockchain.at(blockchain.size() - 1).getHash(), currentDate);

        // Mine the new block
        newBlock.mineBlock(prefix);

        // Validate new block
        while(!newBlock.validateBlock(blockchain, prefix))
        {
            // If our validation fails, retry mining after resetting values.
            newBlock.setNonce(0);
            long currentDate = (chrono::duration_cast<chrono::milliseconds>((chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now())).time_since_epoch())).count();
            newBlock.setTimeStamp(currentDate);
            newBlock.calculateBlockHash();
            newBlock.mineBlock(prefix);
        }

        // Add our newly mined and verified block and set up values for next block
        blockchain.push_back(newBlock);

        // End timer for this block
        long endBlock = (chrono::duration_cast<chrono::microseconds>((chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())).time_since_epoch())).count();

        // Calculate execution time for block
        float durationBlock = (endBlock - startBlock) / 1000.f;

        // Add duration to vector
        executionTimes.push_back(durationBlock);
    }

    // Close input file
    inputFile.close();

    // Stop timer
    long stopBlockchain = (chrono::duration_cast<chrono::microseconds>((chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now())).time_since_epoch())).count();
    if (fullOutput)
    {
        cout << "All blocks added!" << endl << endl;
    }

    // Calculate total execution time
    float durationBlockchain = (stopBlockchain - startBlockchain) / 1000.f;

    // Print block execution times to txt file & calculate interesting data features
    float sum = 0.0;
    float averageBlockTime = 0;
    float fastestBlockTime = 100000000.0;
    float slowestBlockTime = 0.0;

    for (int dataEntryIndex = 0; dataEntryIndex < executionTimes.size(); dataEntryIndex++)
    {

        // Add execution time to sum
        sum += executionTimes.at(dataEntryIndex);

        // Get fastest execution time
        if (executionTimes.at(dataEntryIndex) < fastestBlockTime)
        {
            fastestBlockTime = executionTimes.at(dataEntryIndex);
        }

        // Get slowest execution time
        if (executionTimes.at(dataEntryIndex) > slowestBlockTime)
        {
            slowestBlockTime = executionTimes.at(dataEntryIndex);
        }
    }

    // Calculate average
    averageBlockTime = sum / (executionTimes.size());

    // Display interesting data points.
    if (fullOutput)
    {
        cout << "=================== Data ===================" << endl;
        cout << setw(32) << left << "Number of blocks added:" << right << to_string(executionTimes.size()) << endl;
        cout << setw(32) << left << "Total execution time:" << right << to_string(durationBlockchain) + " ms." << endl;
        cout << setw(32) << left << "Average block execution time:" << right << to_string(averageBlockTime) + " ms." << endl;
        cout << setw(32) << left << "Fastest block execution time:" << right << to_string(fastestBlockTime) + " ms." << endl;
        cout << setw(32) << left << "Slowest block execution time:" << right << to_string(slowestBlockTime) + " ms." << endl;
    }
    else
    {
        cout << durationBlockchain << " " << averageBlockTime << endl;
    }

    return 0;
}
