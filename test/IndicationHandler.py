import pyprovider, pywbem

def handle_indication(env, ns, handlerInstance, indicationInstance):
    logger = env.get_logger();
    logger.log_debug('#### Python handle_indication called')
    print '######## Handler Instance:'
    print handlerInstance.tocimxml().toprettyxml()
    print '######## Source Instance:'
    ci = indicationInstance['SourceInstance'];
    print ci.tocimxml().toprettyxml()
    print '######## Previous Instance:'
    ci = indicationInstance['PreviousInstance'];
    print ci.tocimxml().toprettyxml()
    logger.log_debug('#### Python handle_indication returning')

