function [backoff_start, phy_tx_start, tx_done, fsm_state_out] = ...
    mac_tx_ctrl_c_fsm(...
        reset, ...
        new_tx, ...
        require_backoff, ...
        backoff_done, ...
        idle_for_difs, ...
        phy_tx_done)

persistent fsm_state, fsm_state=xl_state(0, {xlUnsigned, 3, 0});

fsm_state_out = fsm_state;

%Inputs:
% reset: synchronous reset, forces internal state variables back to default (IDLE state)
% new_tx: Software request for new Tx cycle
% backoff_done: Indication from MAC hw that backoff period is done
% idle_for_difs: Indication from MAC hw that medium has been idle > DIFS/EIFS
% phy_tx_done: Indication from PHY that last sample is transmitted

%Outputs:
% backoff_start: Indication to MAC hw to run idle->backoff process
% phy_tx_start: Indication to PHY to start Tx
% tx_done: Indication to MAC hw that this Tx cycle is complete
% fsm_state_out: Value of  internal state register (for debugging)

ST_IDLE = 0;
ST_START_BO = 1;
ST_DEFER = 2;
ST_DO_TX = 3;
ST_DONE = 4;

if(reset)
    backoff_start = 0;
    phy_tx_start = 0;
    tx_done = 0;
    fsm_state = ST_IDLE;

else
    switch double(fsm_state)

        case ST_IDLE
            backoff_start = 0;
            phy_tx_start = 0;
            tx_done = 0;

            if(new_tx)
                if(idle_for_difs && ~require_backoff)
                    %If medium is already idle and this Tx doesn't require a backoff, start Tx immediately
                    fsm_state = ST_DO_TX;
                else
                    %Busy medium, or backoff required; start backoff
                    fsm_state = ST_START_BO;
                end
            else
                %No new Tx this cycle
                fsm_state = ST_IDLE;
            end

        case ST_START_BO
            %Start the backoff counter, then transition to DEFER
            backoff_start = 1;
            phy_tx_start = 0;
            tx_done = 0;

            fsm_state = ST_DEFER;

        case ST_DEFER
            backoff_start = 0;
            phy_tx_start = 0;
            tx_done = 0;

            %Stay here until backoff completes
            if(backoff_done)
                fsm_state = ST_DO_TX;
            else
                fsm_state = ST_DEFER;
            end

        case ST_DO_TX
            backoff_start = 0;
            phy_tx_start = 1;
            tx_done = 0;

            % Stay here until PHY Tx finishes
            if(phy_tx_done)
                fsm_state = ST_DONE;
            else
                fsm_state = ST_DO_TX;
            end

        case ST_DONE
            backoff_start = 0;
            phy_tx_start = 0;
            tx_done = 1;

            fsm_state = ST_IDLE;

        otherwise
            %This case should be impossible; mostly here to appease MATLAB
            backoff_start = 0;
            phy_tx_start = 0;
            tx_done = 0;
            fsm_state = ST_IDLE;

    end %end switch
end %end else

end %end function
