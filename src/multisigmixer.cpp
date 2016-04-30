//*****************************************************************************
//*****************************************************************************

#include "multisigmixer.h"

CMultisigTx* msgtx;
CCriticalSection cs_msgtx;

//*****************************************************************************
//*****************************************************************************
void start_distmix_search(int64 amount, ptime start)
{
    CMultisigSearch msgsearch;
    msgsearch.amount = amount;

    bool found_next = false;
	

    while (second_clock::universal_time() - start <
                        boost::posix_time::seconds(CMultisigTx::networkTimeout))
    {
        std::cerr << "Time waited: " << second_clock::universal_time() - start << std::endl;
        std::cerr << "Is it done?" << (second_clock::universal_time() - start >= posix_time::seconds(60)) << std::endl;
        std::cerr << "Start while(true)!" << std::endl;

        {
            LOCK(cs_msgtx);

            if (!msgtx)
            {
                // dropped
                return;
            }

            if (msgtx->root == false)
            {
                std::cerr << "No longer root; transferring ownership." << std::endl;
                found_next = true;
            }
        }

        std::cerr << "Finished root check" << std::endl;

        if (found_next)
        {
            break;
        }

        {
            LOCK(cs_vNodes);
            std:: cerr << "Broadcasting!" << std::endl;
            BOOST_FOREACH(CNode* pnode, vNodes)
            {
                pnode->PushMessage("mssearch", CMultisigTx::version(), msgsearch);
            }
            std::cerr << "broadcast done" << std::endl;
        }

        uiInterface.NotifyDistmixPaymentStatusChanged("mssearch");

        boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    }

    std::cerr << "Loop over." << std::endl;

    {
        // TODO
        // remove uiInterface.Notify... from under LOCK
        // potentially deadlock

        LOCK(cs_msgtx);

        if (!msgtx)
        {
            // dropped
            return;
        }

        if (found_next == false && msgtx->prev != NULL && msgtx->inputs.size() >= 3)
        {
            uiInterface.NotifyDistmixPaymentStatusChanged("msjoin");

            std::cerr << "Found no successor; starting transaction with "
                      << msgtx->inputs.size() << " inputs." << std::endl;

            // fee for autonodes
            if (msgtx->fee > 0 && msgtx->autonodes.size() > 0)
            {
                boost::int64_t fee = msgtx->fee / msgtx->autonodes.size();

                BOOST_FOREACH(const std::string & node, msgtx->autonodes)
                {
                    msgtx->outputs.push_back(std::pair<std::string, boost::int64_t>(node, fee));
                    msgtx->fee -= fee;
                    if (msgtx->fee <= 0)
                    {
                        break;
                    }
                }
            }

            // create raw transaction and sign it
            CTransaction rawTx;

            for (vector<pair<string, int> >::iterator it = msgtx->inputs.begin(); it != msgtx->inputs.end(); ++it)
            {
                CTxIn in(COutPoint(uint256(at_c<0>(*it)), at_c<1>(*it)));
                rawTx.vin.push_back(in);
            }

            for (vector<pair<string, boost::int64_t> >::iterator it = msgtx->outputs.begin(); it != msgtx->outputs.end(); ++it)
            {
                CBitcoinAddress address(at_c<0>(*it));
                if (!address.IsValid())
                {
                    std::cerr << "Address invalid: " << at_c<0>(*it);
                }

                CScript scriptPubKey;
                scriptPubKey.SetDestination(address.Get());

                CTxOut out(at_c<1>(*it), scriptPubKey);
                rawTx.vout.push_back(out);
            }

            std::random_shuffle(rawTx.vout.begin(), rawTx.vout.end());

            CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
            ss << rawTx;

            Array params;
            params.push_back(HexStr(ss.begin(), ss.end()));

            try
            {
				Object result = signrawtransaction(params, false).get_obj();

                CMultisigSign msgsign;
                msgsign.amount = msgtx->amount;
                msgsign.rawtx = result[0].value_.get_str();
                msgtx->prev->PushMessage("mssign", CMultisigTx::version(), msgsign);
            }
			catch (Object& e)
			{
				std::cerr << "FATAL:" << std::endl;
				for (Object::iterator i = e.begin(); i != e.end(); ++i)
				{
					if ((*i).value_.type() == str_type)
					{
						std::cerr << ">" << (*i).value_.get_str() << std::endl;
					}
				}

				uiInterface.NotifyDistmixPaymentStatusChanged("msfailure");

				if (msgtx->next != NULL)
				{
					msgtx->next->PushMessage("msfailure");
				}

				if (msgtx->prev != NULL)
				{
					msgtx->prev->PushMessage("msfailure");
				}

				delete msgtx;
				msgtx = NULL;
			}
        }

        else if (found_next == false)
        {
            uiInterface.NotifyDistmixPaymentStatusChanged("msfailure");

            if (msgtx->prev)
            {
                msgtx->prev->PushMessage("msfailure");
            }

            std::cerr << "FAILED TO COMPLETE MULTISIG TRANSACTION WITHOUT ENOUGH USERS"
                      << std::endl;
            std::cerr << "inputs: " << msgtx->inputs.size()
                      << " and outputs: " << msgtx->outputs.size() << std::endl;

            delete msgtx;
            msgtx = NULL;
        }
        else
        {
            uiInterface.NotifyDistmixPaymentStatusChanged("msprocess");
        }
    }

    // check finished and wait if not

    std::cerr << "Wait for end of distmix" << std::endl;

    while (second_clock::universal_time() - start <
                    boost::posix_time::seconds(CMultisigTx::dropSessionTimeout))
    {
        {
            LOCK(cs_msgtx);

            if (!msgtx)
            {
                std::cerr << "Done with distmix" << std::endl;
                return;
            }
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    }

    std::cerr << "Distmix timeout expired" << std::endl;

    {
        LOCK(cs_msgtx);

        delete msgtx;
        msgtx = 0;
    }

    uiInterface.NotifyDistmixPaymentStatusChanged("mstimeout");
}

//*****************************************************************************
//*****************************************************************************
void wait_for_distmix_search()
{
    uiInterface.NotifyDistmixPaymentStatusChanged("mssearch");

    bool isAutonode = GetBoolArg("-distmix-autonode");

    // this_thread::sleep(boost::posix_time::seconds(isAutonode ? 15 : 5));
    this_thread::sleep(boost::posix_time::seconds(5));

    bool joining, joined;
    int64 amount;
    ptime start;

    {
        LOCK(cs_msgtx);

        if (!msgtx)
        {
            return;
        }

        // drop session for autonode if not joined
        if (isAutonode && !msgtx->joined)
        {
            delete msgtx;
            msgtx = 0;
            return;
        }

        joining = msgtx->joining;
        joined  = msgtx->joined;
        amount  = msgtx->amount;
        start   = msgtx->start;

        if (!joining && !joined)
        {
            msgtx->root    = true;
            msgtx->joined  = true;
            msgtx->joining = false;
            msgtx->prev    = NULL;
            msgtx->inputs  = msgtx->myinputs;
            msgtx->outputs = msgtx->myoutputs;
        }

        if (joining)
        {
            msgtx->joining = false;
        }
    }

    if (!joining && !joined)
    {
        std::cerr << "starting search, root = " << msgtx->root << std::endl;
        start_distmix_search(amount, start);
    }

    else if (!joined)
    {
        wait_for_distmix_search();
    }

    else
    {
        std::cerr << "success! now becoming root: " << msgtx->root << std::endl;
        start_distmix_search(amount, start);
    }
}
